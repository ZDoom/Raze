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

#include "duke3d.h"
#include "demo.h"
#include "d_event.h"
#include "gamevar.h"

BEGIN_DUKE_NS


int32_t PHEIGHT = PHEIGHT_DUKE;

int32_t lastvisinc;
hudweapon_t hudweap;

#ifdef SPLITSCREEN_MOD_HACKS
static int32_t g_snum;
#endif

extern int32_t g_levelTextTime, ticrandomseed;

int32_t g_numObituaries = 0;
int32_t g_numSelfObituaries = 0;


int const icon_to_inv[ICON_MAX] = { GET_FIRSTAID, GET_FIRSTAID, GET_STEROIDS, GET_HOLODUKE,
                                    GET_JETPACK,  GET_HEATS,    GET_SCUBA,    GET_BOOTS };

int const inv_to_icon[GET_MAX] = { ICON_STEROIDS, ICON_NONE,  ICON_SCUBA, ICON_HOLODUKE, ICON_JETPACK, ICON_NONE,
                                   ICON_NONE,     ICON_HEATS, ICON_NONE,  ICON_FIRSTAID, ICON_BOOTS };

void P_AddKills(DukePlayer_t * const pPlayer, uint16_t kills)
{
    pPlayer->actors_killed += kills;
}

void P_UpdateScreenPal(DukePlayer_t * const pPlayer)
{
    int       inWater       = 0;
    int const playerSectnum = pPlayer->cursectnum;

    if (pPlayer->DrugMode)
        pPlayer->palette = DRUGPAL;
    else if (pPlayer->heat_on)
        pPlayer->palette = SLIMEPAL;
    else if (playerSectnum < 0)
        pPlayer->palette = BASEPAL;
    else if (sector[playerSectnum].ceilingpicnum >= TILE_FLOORSLIME && sector[playerSectnum].ceilingpicnum <= TILE_FLOORSLIME + 2)
    {
        pPlayer->palette = SLIMEPAL;
        inWater          = 1;
    }
    else
    {
        pPlayer->palette     = (sector[pPlayer->cursectnum].lotag == ST_2_UNDERWATER) ? WATERPAL : BASEPAL;
        inWater              = 1;
    }

    restorepalette = 1+inWater;
}

static void P_IncurDamage(DukePlayer_t * const pPlayer)
{
    sprite[pPlayer->i].extra -= pPlayer->extra_extra8>>8;

    int playerDamage = sprite[pPlayer->i].extra - pPlayer->last_extra;

    if (playerDamage >= 0)
        return;

    pPlayer->extra_extra8 = 0;

    if ((!RR && pPlayer->inv_amount[GET_SHIELD] > 0) || (RR && pPlayer->inv_amount[GET_STEROIDS] > 0 && pPlayer->inv_amount[GET_STEROIDS] < 400))
    {
        int const shieldDamage = playerDamage * (20 + (krand2()%30)) / 100;

        playerDamage                     -= shieldDamage;
        if (!RR)
        {
            pPlayer->inv_amount[GET_SHIELD] += shieldDamage;

            if (pPlayer->inv_amount[GET_SHIELD] < 0)
            {
                playerDamage += pPlayer->inv_amount[GET_SHIELD];
                pPlayer->inv_amount[GET_SHIELD] = 0;
            }
        }
    }

    if (RR)
    {
        int guts = 0;
        if (pPlayer->drink_amt > 31 && pPlayer->drink_amt < 65)
            guts++;
        if (pPlayer->eat > 31 && pPlayer->eat < 65)
            guts++;

        switch (guts)
        {
            case 1:
                playerDamage = (int)(playerDamage*0.75);
                break;
            case 2:
                playerDamage = (int)(playerDamage*0.25);
                break;
        }
    }

    sprite[pPlayer->i].extra = pPlayer->last_extra + playerDamage;
}


//////////////////// HUD WEAPON / MISC. DISPLAY CODE ////////////////////

static void P_DisplaySpit(void)
{
    DukePlayer_t *const pPlayer     = g_player[screenpeek].ps;
    int const           loogCounter = pPlayer->loogcnt;

    if (loogCounter == 0)
        return;

    int const rotY = loogCounter<<2;

    for (bssize_t i=0; i < pPlayer->numloogs; i++)
    {
        int const rotAng = klabs(sintable[((loogCounter + i) << 5) & 2047]) >> 5;
        int const rotZoom  = 4096 + ((loogCounter + i) << 9);
        int const rotX     = (-fix16_to_int(g_player[screenpeek].input->q16avel) >> 1) + (sintable[((loogCounter + i) << 6) & 2047] >> 10);

        rotatesprite_fs((pPlayer->loogiex[i] + rotX) << 16, (200 + pPlayer->loogiey[i] - rotY) << 16, rotZoom - (i << 8),
                        256 - rotAng, TILE_LOOGIE, 0, 0, 2);
    }
}

int P_GetHudPal(const DukePlayer_t *p)
{
    if (sprite[p->i].pal == 1)
        return 1;

    if (p->cursectnum >= 0)
    {
        int const hudPal = sector[p->cursectnum].floorpal;
        if (!lookups.noFloorPal(hudPal))
            return hudPal;
    }

    return 0;
}

int P_GetKneePal(DukePlayer_t const * pPlayer)
{
    return P_GetKneePal(pPlayer, P_GetHudPal(pPlayer));
}

int P_GetKneePal(DukePlayer_t const * pPlayer, int const hudPal)
{
    return hudPal == 0 ? pPlayer->palookup : hudPal;
}

int P_GetOverheadPal(DukePlayer_t const * pPlayer)
{
    return sprite[pPlayer->i].pal;
}

static int P_DisplayFist(int const fistShade)
{
    DukePlayer_t const *const pPlayer = g_player[screenpeek].ps;
    int fistInc = pPlayer->fist_incs;

    if (fistInc > 32)
        fistInc = 32;

    if (fistInc <= 0)
        return 0;

    int const fistY       = klabs(pPlayer->look_ang) / 9;
    int const fistZoom    = clamp(65536 - (sintable[(512 + (fistInc << 6)) & 2047] << 2), 40920, 90612);
    int const fistYOffset = 194 + (sintable[((6 + fistInc) << 7) & 2047] >> 9);
    int const fistPal     = P_GetHudPal(pPlayer);
    int       wx[2]       = { windowxy1.x, windowxy2.x };

#ifdef SPLITSCREEN_MOD_HACKS
    // XXX: this is outdated, doesn't handle above/below split.
    if (g_fakeMultiMode==2)
        wx[(g_snum==0)] = (wx[0]+wx[1])/2+1;
#endif

    rotatesprite((-fistInc + 222 + (fix16_to_int(g_player[screenpeek].input->q16avel) >> 5)) << 16, (fistY + fistYOffset) << 16,
                 fistZoom, 0, TILE_FIST, fistShade, fistPal, 2, wx[0], windowxy1.y, wx[1], windowxy2.y);

    return 1;
}

#define DRAWEAP_CENTER 262144
#define weapsc(sc) scale(sc, hud_weaponscale, 100)

static int32_t g_dts_yadd;

static void G_DrawTileScaled(int drawX, int drawY, int tileNum, int drawShade, int drawBits, int drawPal,
    int drawScale = 65536, int angleOffset = 0)
{
    int32_t wx[2] = { windowxy1.x, windowxy2.x };
    int32_t wy[2] = { windowxy1.y, windowxy2.y };

    int drawYOffset = 0;
    int drawXOffset = 192<<16;

    switch (DYNAMICWEAPONMAP(hudweap.cur))
    {
        case DEVISTATOR_WEAPON__STATIC:
        case TRIPBOMB_WEAPON__STATIC:
            drawXOffset = 160<<16;
            break;
        default:
            if (drawBits & DRAWEAP_CENTER)
            {
                drawXOffset = 160<<16;
                drawBits &= ~DRAWEAP_CENTER;
            }
            break;
    }

    // bit 4 means "flip x" for G_DrawTileScaled
    int const drawAng = ((drawBits & 4) ? 1024 : 0) + angleOffset;

#ifdef SPLITSCREEN_MOD_HACKS
    if (g_fakeMultiMode==2)
    {
        int const sideBySide = (ud.screen_size!=0);

        // splitscreen HACK
        drawBits &= ~(1024|512|256);
        if (sideBySide)
        {
            drawBits &= ~8;
            wx[(g_snum==0)] = (wx[0]+wx[1])/2 + 2;
        }
        else
        {
            drawBits |= 8;
            if (g_snum==0)
                drawYOffset = -(100<<16);
            wy[(g_snum==0)] = (wy[0]+wy[1])/2 + 2;
        }
    }
#endif

#ifdef USE_OPENGL
    if (videoGetRenderMode() >= REND_POLYMOST && hw_models && md_tilehasmodel(tileNum,drawPal) >= 0)
        drawYOffset += (224<<16)-weapsc(224<<16);
#endif
    rotatesprite(weapsc(drawX<<16) + (drawXOffset-weapsc(drawXOffset)),
                 weapsc((drawY<<16) + g_dts_yadd) + ((200<<16)-weapsc(200<<16)) + drawYOffset,
                 weapsc(drawScale),drawAng,tileNum,drawShade,drawPal,(2|drawBits),
                 wx[0],wy[0], wx[1],wy[1]);
}

static void G_DrawWeaponTile(int weaponX, int weaponY, int weaponTile, int weaponShade, int weaponBits, int weaponPal, int weaponScale = 65536)
{
    static int shadef = 0;
    static int palf = 0;

    // basic fading between player weapon shades
    if (shadef != weaponShade && (!weaponPal || palf == weaponPal))
    {
        shadef += (weaponShade - shadef) >> 2;

        if (!((weaponShade - shadef) >> 2))
            shadef = logapproach(shadef, weaponShade);
    }
    else
        shadef = weaponShade;

    palf = weaponPal;

#ifdef USE_OPENGL
    if (videoGetRenderMode() >= REND_POLYMOST)
    {
        if (!RR && weaponTile >= TILE_CHAINGUN + 1 && weaponTile <= TILE_CHAINGUN + 4)
        {
            if (!hw_models || md_tilehasmodel(weaponTile, weaponPal) < 0)
            {
                // HACK: Draw the upper part of the chaingun two screen
                // pixels (not texels; multiplied by weapon scale) lower
                // first, preventing ugly horizontal seam.
                g_dts_yadd = tabledivide32_noinline(65536 * 2 * 200, ydim);
                G_DrawTileScaled(weaponX, weaponY, weaponTile, shadef, weaponBits, weaponPal);
                g_dts_yadd = 0;
            }
        }
    }
#endif

    G_DrawTileScaled(weaponX, weaponY, weaponTile, shadef, weaponBits, weaponPal, weaponScale);
}

static inline void G_DrawWeaponTileWithID(int uniqueID, int weaponX, int weaponY, int weaponTile, int weaponShade,
                                          int weaponBits, int p, int weaponScale = 65536)
{
    int lastUniqueID = guniqhudid;
    guniqhudid       = uniqueID;

    G_DrawWeaponTile(weaponX, weaponY, weaponTile, weaponShade, weaponBits, p, weaponScale);

    guniqhudid       = lastUniqueID;
}

static inline void G_DrawWeaponTileUnfadedWithID(int uniqueID, int weaponX, int weaponY, int weaponTile, int weaponShade,
                                          int weaponBits, int p)
{
    int lastUniqueID = guniqhudid;
    guniqhudid       = uniqueID;

    G_DrawTileScaled(weaponX, weaponY, weaponTile, weaponShade, weaponBits, p); // skip G_DrawWeaponTile

    guniqhudid       = lastUniqueID;
}

static int P_DisplayKnee(int kneeShade)
{
    static int8_t const       knee_y[] = { 0, -8, -16, -32, -64, -84, -108, -108, -108, -72, -32, -8 };
    const DukePlayer_t *const ps = g_player[screenpeek].ps;

    if (ps->knee_incs == 0)
        return 0;

    if (ps->knee_incs >= ARRAY_SIZE(knee_y) || sprite[ps->i].extra <= 0)
        return 0;

    int const kneeY   = knee_y[ps->knee_incs] + (klabs(ps->look_ang) / 9) - (ps->hard_landing << 3);
    int const kneePal = P_GetKneePal(ps);

    G_DrawTileScaled(105+(fix16_to_int(g_player[screenpeek].input->q16avel)>>5)-(ps->look_ang>>1)+(knee_y[ps->knee_incs]>>2),
                     kneeY+280-(fix16_to_int(ps->q16horiz-ps->q16horizoff)>>4),TILE_KNEE,kneeShade,4+DRAWEAP_CENTER,kneePal);

    return 1;
}

static int P_DisplayKnuckles(int knuckleShade)
{
    if (WW2GI)
        return 0;
    const DukePlayer_t *const pPlayer = g_player[screenpeek].ps;

    if (pPlayer->knuckle_incs == 0)
        return 0;

    static int8_t const knuckleFrames[] = { 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0 };

    if ((unsigned) (pPlayer->knuckle_incs>>1) >= ARRAY_SIZE(knuckleFrames) || sprite[pPlayer->i].extra <= 0)
        return 0;

    int const knuckleY   = (klabs(pPlayer->look_ang) / 9) - (pPlayer->hard_landing << 3);
    int const knucklePal = P_GetHudPal(pPlayer);

    G_DrawTileScaled(160 + (fix16_to_int(g_player[screenpeek].input->q16avel) >> 5) - (pPlayer->look_ang >> 1),
                     knuckleY + 180 - (fix16_to_int(pPlayer->q16horiz - pPlayer->q16horizoff) >> 4),
                     TILE_CRACKKNUCKLES + knuckleFrames[pPlayer->knuckle_incs >> 1], knuckleShade, 4 + DRAWEAP_CENTER,
                     knucklePal);

    return 1;
}

// Set C-CON's WEAPON and WORKSLIKE gamevars.
void P_SetWeaponGamevars(int playerNum, const DukePlayer_t * const pPlayer)
{
    if (!WW2GI)
        return;
    SetGameVarID(g_iWeaponVarID, pPlayer->curr_weapon, pPlayer->i, playerNum);
    SetGameVarID(g_iWorksLikeVarID,
              ((unsigned)pPlayer->curr_weapon < MAX_WEAPONS) ? PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) : -1,
              pPlayer->i, playerNum);
}

static void P_FireWeapon(int playerNum)
{
    auto const pPlayer = g_player[playerNum].ps;

    if (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) != KNEE_WEAPON)
        pPlayer->ammo_amount[pPlayer->curr_weapon]--;

    if (PWEAPON(playerNum, pPlayer->curr_weapon, FireSound) > 0)
        A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, FireSound), pPlayer->i);

    P_SetWeaponGamevars(playerNum, pPlayer);
    //        Printf("doing %d %d %d\n",PWEAPON(snum, p->curr_weapon, Shoots),p->curr_weapon,snum);
    fi.shoot(pPlayer->i, PWEAPON(playerNum, pPlayer->curr_weapon, Shoots));

    for (bssize_t burstFire = PWEAPON(playerNum, pPlayer->curr_weapon, ShotsPerBurst) - 1; burstFire > 0; --burstFire)
    {
        fi.shoot(pPlayer->i, PWEAPON(playerNum, pPlayer->curr_weapon, Shoots));

        if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_AMMOPERSHOT)
        {
            pPlayer->ammo_amount[pPlayer->curr_weapon]--;
        }
    }

    if (!(PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_NOVISIBLE))
    {
#ifdef POLYMER
        spritetype *s = &sprite[pPlayer->i];
        int32_t     x = ((sintable[(s->ang + 512) & 2047]) >> 7), y = ((sintable[(s->ang) & 2047]) >> 7);

        s->x += x;
        s->y += y;
        G_AddGameLight(0, pPlayer->i, PHEIGHT, 8192, PWEAPON(playerNum, pPlayer->curr_weapon, FlashColor),
                       PR_LIGHT_PRIO_MAX_GAME);
        actor[pPlayer->i].lightcount = 2;
        s->x -= x;
        s->y -= y;
#endif  // POLYMER
        pPlayer->visibility = 0;
    }

    if (/*!(PWEAPON(playerNum, p->curr_weapon, Flags) & WEAPON_CHECKATRELOAD) && */
            PWEAPON(playerNum, pPlayer->curr_weapon, Reload) > PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime) && pPlayer->ammo_amount[pPlayer->curr_weapon] > 0
                && (PWEAPON(playerNum, pPlayer->curr_weapon, Clip)) && (((pPlayer->ammo_amount[pPlayer->curr_weapon]%(PWEAPON(playerNum, pPlayer->curr_weapon, Clip)))==0)))
    {
        pPlayer->kickback_pic = PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime);
    }

    if (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) != KNEE_WEAPON)
        P_CheckWeapon(pPlayer);
}

static void P_DoWeaponSpawn(int playerNum)
{
    auto const pPlayer = g_player[playerNum].ps;

    // NOTE: For the 'Spawn' member, 0 means 'none', too (originally so,
    // i.e. legacy). The check for <0 was added to the check because mod
    // authors (rightly) assumed that -1 is the no-op value.
    if (PWEAPON(playerNum, pPlayer->curr_weapon, Spawn) <= 0)  // <=0 : AMC TC beta/RC2 has WEAPONx_SPAWN -1
        return;

    int newSprite = fi.spawn(pPlayer->i, PWEAPON(playerNum, pPlayer->curr_weapon, Spawn));
    
    if ((PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_SPAWNTYPE2))
    {
        // like shotgun shells
        sprite[newSprite].ang += 1024;
        A_SetSprite(newSprite,CLIPMASK0);
        sprite[newSprite].ang += 1024;
    }
    else if ((PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_SPAWNTYPE3))
    {
        // like chaingun shells
        sprite[newSprite].ang += 1024;
        sprite[newSprite].ang &= 2047;
        sprite[newSprite].xvel += 32;
        sprite[newSprite].z += (3<<8);
        A_SetSprite(newSprite,CLIPMASK0);
    }
}

void P_DisplayScuba(void)
{
    if (g_player[screenpeek].ps->scuba_on)
    {
        const DukePlayer_t *const pPlayer = g_player[screenpeek].ps;

        int const scubaPal = P_GetHudPal(pPlayer);

#ifdef SPLITSCREEN_MOD_HACKS
        g_snum = screenpeek;
#endif

        if (RR)
        {

            if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING))
            {
                G_DrawTileScaled(320 - (tilesiz[TILE_SCUBAMASK].x >> 1) - 15, 200 - (tilesiz[TILE_SCUBAMASK].y >> 1) + (sintable[(int32_t) totalclock & 2047] >> 10),
                                 TILE_SCUBAMASK, 0, 2 + 16 + DRAWEAP_CENTER+512, scubaPal,49152);
                G_DrawTileScaled(320 - tilesiz[TILE_SCUBAMASK+4].x, 200 - tilesiz[TILE_SCUBAMASK+4].y, TILE_SCUBAMASK+4, 0, 2 + 16 + DRAWEAP_CENTER + 1024, scubaPal);
                G_DrawTileScaled(tilesiz[TILE_SCUBAMASK+4].x, 200 - tilesiz[TILE_SCUBAMASK+4].y, TILE_SCUBAMASK+4, 0, 2 + 4 + 16 + DRAWEAP_CENTER + 1024, scubaPal);
                //G_DrawTileScaled(35, -1, TILE_SCUBAMASK+3, 0, 2 + 16 + DRAWEAP_CENTER+512, scubaPal);
                //G_DrawTileScaled(35, -1, TILE_SCUBAMASK+3, 0, 2 + 16 + DRAWEAP_CENTER+256, scubaPal);
                //G_DrawTileScaled(285, 200, TILE_SCUBAMASK+3, 0, 2 + 16 + DRAWEAP_CENTER+512, scubaPal,65536,1024);
                //G_DrawTileScaled(285, 200, TILE_SCUBAMASK+3, 0, 2 + 16 + DRAWEAP_CENTER+256, scubaPal,65536,1024);
                G_DrawTileScaled(35, -1, TILE_SCUBAMASK+3, 0, 2 + 16 + DRAWEAP_CENTER+1024, scubaPal);
                G_DrawTileScaled(285, 200, TILE_SCUBAMASK+3, 0, 2 + 16 + DRAWEAP_CENTER+1024, scubaPal,65536,1024);
            }
            else
            {
                G_DrawTileScaled(320 - (tilesiz[TILE_SCUBAMASK].x >> 1) - 15, 200 - (tilesiz[TILE_SCUBAMASK].y >> 1) + (sintable[(int32_t) totalclock & 2047] >> 10),
                                 TILE_SCUBAMASK, 0, 2 + 16 + DRAWEAP_CENTER, scubaPal,49152);
                G_DrawTileScaled(320 - tilesiz[TILE_SCUBAMASK+4].x, 200 - tilesiz[TILE_SCUBAMASK+4].y, TILE_SCUBAMASK+4, 0, 2 + 16 + DRAWEAP_CENTER, scubaPal);
                G_DrawTileScaled(tilesiz[TILE_SCUBAMASK+4].x, 200 - tilesiz[TILE_SCUBAMASK+4].y, TILE_SCUBAMASK+4, 0, 2 + 4 + 16 + DRAWEAP_CENTER, scubaPal);
                G_DrawTileScaled(35, -1, TILE_SCUBAMASK+3, 0, 2 + 16 + DRAWEAP_CENTER, scubaPal);
                G_DrawTileScaled(285, 200, TILE_SCUBAMASK+3, 0, 2 + 16 + DRAWEAP_CENTER, scubaPal,65536,1024);
            }
        }
        else
        {
            // this is a hack to hide the seam that appears between the two halves of the mask in GL
#ifdef USE_OPENGL
            if (videoGetRenderMode() >= REND_POLYMOST)
                G_DrawTileScaled(44, (200 - tilesiz[TILE_SCUBAMASK].y), TILE_SCUBAMASK, 0, 2 + 16 + DRAWEAP_CENTER, scubaPal);
#endif
            G_DrawTileScaled(43, (200 - tilesiz[TILE_SCUBAMASK].y), TILE_SCUBAMASK, 0, 2 + 16 + DRAWEAP_CENTER, scubaPal);
            G_DrawTileScaled(320 - 43, (200 - tilesiz[TILE_SCUBAMASK].y), TILE_SCUBAMASK, 0, 2 + 4 + 16 + DRAWEAP_CENTER, scubaPal);
        }
    }
}

static int8_t const access_tip_y [] = {
    0, -8, -16, -32, -64, -84, -108, -108, -108, -108, -108, -108, -108, -108, -108, -108, -96, -72, -64, -32, -16
};

static int P_DisplayTip(int tipShade)
{
    const DukePlayer_t *const pPlayer = g_player[screenpeek].ps;

    if (pPlayer->tipincs == 0)
        return 0;

    // Report that the tipping hand has been drawn so that the otherwise
    // selected weapon is not drawn.
    if ((unsigned)pPlayer->tipincs >= ARRAY_SIZE(access_tip_y))
        return 1;

    int const tipY       = (klabs(pPlayer->look_ang) / 9) - (pPlayer->hard_landing << 3);
    int const tipPal     = P_GetHudPal(pPlayer);
    int const tipYOffset = access_tip_y[pPlayer->tipincs] >> 1;

    guniqhudid = 201;

    G_DrawTileScaled(170 + (fix16_to_int(g_player[screenpeek].input->q16avel) >> 5) - (pPlayer->look_ang >> 1),
                     tipYOffset + tipY + 240 - (fix16_to_int(pPlayer->q16horiz - pPlayer->q16horizoff) >> 4),
                     TILE_TIP + ((26 - pPlayer->tipincs) >> 4), tipShade, DRAWEAP_CENTER, tipPal);

    guniqhudid = 0;

    return 1;
}

static int P_DisplayAccess(int accessShade)
{
    const DukePlayer_t *const pSprite = g_player[screenpeek].ps;

    if (pSprite->access_incs == 0)
        return 0;

    if ((unsigned)pSprite->access_incs >= ARRAY_SIZE(access_tip_y)-4 || sprite[pSprite->i].extra <= 0)
        return 1;

    int const accessX   = access_tip_y[pSprite->access_incs] >> 2;
    int const accessY   = access_tip_y[pSprite->access_incs] + (klabs(pSprite->look_ang) / 9) - (pSprite->hard_landing << 3);
    int const accessPal = (pSprite->access_spritenum >= 0) ? sprite[pSprite->access_spritenum].pal : 0;

    guniqhudid = 200;

    if ((pSprite->access_incs - 3) > 0 && (pSprite->access_incs - 3) >> 3)
    {
        G_DrawTileScaled(170 + (fix16_to_int(g_player[screenpeek].input->q16avel) >> 5) - (pSprite->look_ang >> 1) + accessX,
                         accessY + 266 - (fix16_to_int(pSprite->q16horiz - pSprite->q16horizoff) >> 4),
                         TILE_HANDHOLDINGLASER + (pSprite->access_incs >> 3), accessShade, DRAWEAP_CENTER, accessPal);
    }
    else
    {
        G_DrawTileScaled(170 + (fix16_to_int(g_player[screenpeek].input->q16avel) >> 5) - (pSprite->look_ang >> 1) + accessX,
                         accessY + 266 - (fix16_to_int(pSprite->q16horiz - pSprite->q16horizoff) >> 4), TILE_HANDHOLDINGACCESS, accessShade,
                         4 + DRAWEAP_CENTER, accessPal);
    }

    guniqhudid = 0;

    return 1;
}

void P_DisplayWeapon(void)
{
    DukePlayer_t *const  pPlayer     = g_player[screenpeek].ps;
    const uint8_t *const weaponFrame = &pPlayer->kickback_pic;

    int currentWeapon, quickKickFrame;

#ifdef SPLITSCREEN_MOD_HACKS
    g_snum = screenpeek;
#endif


    if (pPlayer->newowner >= 0 || ud.camerasprite >= 0 || (!RR && pPlayer->over_shoulder_on > 0)
        || (sprite[pPlayer->i].pal != 1 && sprite[pPlayer->i].extra <= 0))
        return;

    int weaponX       = (160) - 90;
    int weaponY       = klabs(pPlayer->look_ang) / 9;
    int weaponYOffset = 80 - (pPlayer->weapon_pos * pPlayer->weapon_pos);
    int weaponShade   = (RR && pPlayer->cursectnum >= 0 && shadedsector[pPlayer->cursectnum]) ? 16 : (sprite[pPlayer->i].shade <= 24 ? sprite[pPlayer->i].shade : 24);

    int32_t weaponBits = 0;
    UNREFERENCED_PARAMETER(weaponBits);

    if (!RR && (P_DisplayFist(weaponShade) || P_DisplayKnuckles(weaponShade) || P_DisplayTip(weaponShade) || P_DisplayAccess(weaponShade)))
        goto enddisplayweapon;

    if (!RR)
        P_DisplayKnee(weaponShade);

    if (cl_weaponsway)
    {
        weaponX -= (sintable[((pPlayer->weapon_sway>>1)+512)&2047]/(1024+512));
        weaponYOffset -= (sprite[pPlayer->i].xrepeat < (RR ? 8 : 32)) ? klabs(sintable[(pPlayer->weapon_sway << 2) & 2047] >> 9)
                                                           : klabs(sintable[(pPlayer->weapon_sway >> 1) & 2047] >> 10);
    }
    else weaponYOffset -= 16;

    weaponX -= 58 + pPlayer->weapon_ang;
    weaponYOffset -= (pPlayer->hard_landing << 3);

    if (WW2GI)
        currentWeapon   = PWEAPON(screenpeek, (pPlayer->last_weapon >= 0) ? pPlayer->last_weapon : pPlayer->curr_weapon, WorksLike);
    else
        currentWeapon   = (pPlayer->last_weapon >= 0) ? pPlayer->last_weapon : pPlayer->curr_weapon;
    hudweap.gunposy     = weaponYOffset;
    hudweap.lookhoriz   = weaponY;
    hudweap.cur         = currentWeapon;
    hudweap.gunposx     = weaponX;
    hudweap.shade       = weaponShade;
    hudweap.count       = *weaponFrame;
    hudweap.lookhalfang = pPlayer->look_ang >> 1;

    quickKickFrame = 14 - pPlayer->quick_kick;

    if (!RR && (quickKickFrame != 14 || pPlayer->last_quick_kick) && r_drawweapon == 1)
    {
        int const weaponPal = P_GetKneePal(pPlayer);

        guniqhudid = 100;

        if (quickKickFrame < 6 || quickKickFrame > 12)
            G_DrawTileScaled(weaponX + 80 - (pPlayer->look_ang >> 1), weaponY + 250 - weaponYOffset, TILE_KNEE, weaponShade,
                                weaponBits | 4 | DRAWEAP_CENTER, weaponPal);
        else
            G_DrawTileScaled(weaponX + 160 - 16 - (pPlayer->look_ang >> 1), weaponY + 214 - weaponYOffset, TILE_KNEE + 1,
                                weaponShade, weaponBits | 4 | DRAWEAP_CENTER, weaponPal);
        guniqhudid = 0;
    }

    if (RRRA)
    {
        if (pPlayer->OnMotorcycle)
        {
            int motoTile = TILE_MOTOHIT;
            if (!g_netServer && numplayers == 1)
            {
                if (*weaponFrame)
                {
                    weaponShade = 0;
                    if (*weaponFrame == 1)
                    {
                        if ((krand2()&1) == 1)
                            motoTile = TILE_MOTOHIT+1;
                        else
                            motoTile = TILE_MOTOHIT+2;
                    }
                    else if (*weaponFrame == 4)
                    {
                        if ((krand2()&1) == 1)
                            motoTile = TILE_MOTOHIT+3;
                        else
                            motoTile = TILE_MOTOHIT+4;
                    }
                }
            }
            else
            {
                if (*weaponFrame)
                {
                    weaponShade = 0;
                    if (*weaponFrame >= 1 && *weaponFrame <= 4)
                        motoTile += *weaponFrame;
                }
            }

            int const weaponPal = P_GetHudPal(pPlayer);

            G_DrawTileScaled(160-(pPlayer->look_ang>>1), 174, motoTile, weaponShade, 2 | DRAWEAP_CENTER,
                weaponPal, 34816, pPlayer->tilt_status * 5 + (pPlayer->tilt_status < 0 ? 2047 : 0));
            return;
        }
        if (pPlayer->OnBoat)
        {
            int boatTile;
            if (pPlayer->tilt_status > 0)
            {
                if (*weaponFrame == 0)
                    boatTile = TILE_BOATHIT+1;
                else if (*weaponFrame <= 3)
                {
                    boatTile = TILE_BOATHIT+5;
                    weaponShade = -96;
                }
                else if (*weaponFrame <= 6)
                {
                    boatTile = TILE_BOATHIT+6;
                    weaponShade = -96;
                }
                else
                    boatTile = TILE_BOATHIT+1;
            }
            else if (pPlayer->tilt_status < 0)
            {
                if (*weaponFrame == 0)
                    boatTile = TILE_BOATHIT+2;
                else if (*weaponFrame <= 3)
                {
                    boatTile = TILE_BOATHIT+7;
                    weaponShade = -96;
                }
                else if (*weaponFrame <= 6)
                {
                    boatTile = TILE_BOATHIT+8;
                    weaponShade = -96;
                }
                else
                    boatTile = TILE_BOATHIT+2;
            }
            else
            {
                if (*weaponFrame == 0)
                    boatTile = TILE_BOATHIT;
                else if (*weaponFrame <= 3)
                {
                    boatTile = TILE_BOATHIT+3;
                    weaponShade = -96;
                }
                else if (*weaponFrame <= 6)
                {
                    boatTile = TILE_BOATHIT+4;
                    weaponShade = -96;
                }
                else
                    boatTile = TILE_BOATHIT;
            }

            int const weaponPal = P_GetHudPal(pPlayer);
            int weaponY;

            if (pPlayer->not_on_water)
                weaponY = 170;
            else
                weaponY = 170 + (*weaponFrame>>2);

            G_DrawTileScaled(160-(pPlayer->look_ang>>1), weaponY, boatTile, weaponShade, 2 | DRAWEAP_CENTER,
                weaponPal, 66048, pPlayer->tilt_status + (pPlayer->tilt_status < 0 ? 2047 : 0));
            return;
        }
    }

    if (sprite[pPlayer->i].xrepeat < (RR ? 8 : 40))
    {
        static int32_t fistPos;

        int const weaponPal = P_GetHudPal(pPlayer);

        if (pPlayer->jetpack_on == 0)
        {
            int const playerXvel = sprite[pPlayer->i].xvel;
            weaponY += 32 - (playerXvel >> 3);
            fistPos += playerXvel >> 3;
        }

        currentWeapon = weaponX;
        weaponX += sintable[(fistPos)&2047] >> 10;
        G_DrawTileScaled(weaponX + 250 - (pPlayer->look_ang >> 1), weaponY + 258 - (klabs(sintable[(fistPos)&2047] >> 8)),
            TILE_FIST, weaponShade, weaponBits, weaponPal);
        weaponX = currentWeapon - (sintable[(fistPos)&2047] >> 10);
        G_DrawTileScaled(weaponX + 40 - (pPlayer->look_ang >> 1), weaponY + 200 + (klabs(sintable[(fistPos)&2047] >> 8)), TILE_FIST,
            weaponShade, weaponBits | 4, weaponPal);
    }
    else
    {
        switch (r_drawweapon)
        {
            case 1: break;
            case 2:
                if ((unsigned)hudweap.cur < MAX_WEAPONS && hudweap.cur != KNEE_WEAPON)
                    rotatesprite_win(160 << 16, (180 + (pPlayer->weapon_pos * pPlayer->weapon_pos)) << 16, divscale16(ud.statusbarscale, 100), 0,
                                        (!RR && hudweap.cur == GROW_WEAPON) ? TILE_GROWSPRITEICON : WeaponPickupSprites[hudweap.cur], 0,
                                        0, 2);
            default: goto enddisplayweapon;
        }

        if (!RR && currentWeapon == KNEE_WEAPON && *weaponFrame == 0)
            goto enddisplayweapon;

        int const doAnim      = !(sprite[pPlayer->i].pal == 1 || ud.pause_on || g_player[myconnectindex].ps->gm & MODE_MENU);
        int const halfLookAng = pPlayer->look_ang >> 1;

        int const weaponPal = P_GetHudPal(pPlayer);

        if (RR)
        {
            switch (DYNAMICWEAPONMAP(currentWeapon))
            {
            case KNEE_WEAPON__STATIC:
            {
                static int weaponFrames[] = { 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7 };
                static int offsetX[] = { 310,342,364,418,350,316,282,288,0,0 };
                static int offsetY[] = { 300,362,320,268,248,248,277,420,0,0 };
                weaponX = weaponX + ((offsetX[weaponFrames[*weaponFrame]]>>1) - 12);
                weaponY = weaponY + 200 - (244-offsetY[weaponFrames[*weaponFrame]]);
                guniqhudid = currentWeapon;
                G_DrawTileScaled(weaponX - halfLookAng, weaponY - weaponYOffset, TILE_KNEE + weaponFrames[*weaponFrame],
                                    weaponShade, weaponBits, weaponPal, 32768);
                guniqhudid = 0;
                break;
            }
            case SLINGBLADE_WEAPON__STATIC:
                if (RRRA)
                {
                    if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING))
                        weaponBits |= 512;
                    static int weaponFrames[] = { 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7 };
                    static int offsetX[] = { 580,676,310,491,356,210,310,614 };
                    static int offsetY[] = { 369,363,300,323,371,400,300,440 };
                    weaponX = weaponX + ((offsetX[weaponFrames[*weaponFrame]]>>1) - 12);
                    weaponY = weaponY + 210 - (244-offsetY[weaponFrames[*weaponFrame]]);
                    guniqhudid = currentWeapon;
                    G_DrawTileScaled(weaponX - halfLookAng + 20, weaponY - weaponYOffset - 80, TILE_SLINGBLADE + weaponFrames[*weaponFrame],
                                        weaponShade, weaponBits, weaponPal, 32768);
                    guniqhudid = 0;
                    break;
                }
                break;

            case TRIPBOMB_WEAPON__STATIC:
            case BOWLINGBALL_WEAPON__STATIC:
                weaponX += 8;
                weaponYOffset -= 10;

                if (currentWeapon == BOWLING_WEAPON)
                {
                    if (pPlayer->ammo_amount[BOWLING_WEAPON])
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 162 - halfLookAng, weaponY + 214 - weaponYOffset + (*weaponFrame) * 8, TILE_BOWLINGBALLH,
                                                weaponShade, weaponBits, weaponPal, 32768);
                    }
                    else
                    {
                        G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 162 - halfLookAng, weaponY + 214 - weaponYOffset, TILE_HANDTHROW+5,
                                                weaponShade, weaponBits, weaponPal, 36700);
                    }
                }
                else
                {
                    if (pPlayer->ammo_amount[TRIPBOMB_WEAPON])
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 180 - halfLookAng, weaponY + 214 - weaponYOffset + (*weaponFrame) * 8, TILE_POWDERH,
                                                weaponShade, weaponBits, weaponPal, 36700);
                        G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 90 - halfLookAng, weaponY + 214 - weaponYOffset + (*weaponFrame) * 8, TILE_POWDERH,
                                                weaponShade, weaponBits | 4, weaponPal, 36700);
                    }
                    else
                    {
                        G_DrawWeaponTileWithID(currentWeapon << 2, weaponX + 162 - halfLookAng, weaponY + 214 - weaponYOffset, TILE_HANDTHROW+5,
                                                weaponShade, weaponBits, weaponPal, 36700);
                    }
                }
                break;

            case RPG_WEAPON__STATIC:
            {
                if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING))
                    weaponBits |= 512;
                static int frames[] = { 0,1,1,2,2,3,2,3,2,3,2,2,2,2,2,2,2,2,2,4,4,4,4,5,5,5,5,6,6,6,6,6,6,7,7,7,7,7,7 };
                int frame = frames[*weaponFrame];
                if (frame == 2 || frame == 3)
                {
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 200 - halfLookAng, weaponY + 250 - weaponYOffset,
                                           TILE_RPGGUN + frame, weaponShade, weaponBits, weaponPal, 36700);
                }
                else if (frame == 1)
                {
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 200 - halfLookAng, weaponY + 250 - weaponYOffset,
                                           TILE_RPGGUN + frame, 0, weaponBits, weaponPal, 36700);
                }
                else
                {
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 210 - halfLookAng, weaponY + 255 - weaponYOffset,
                                           TILE_RPGGUN + frame, weaponShade, weaponBits, weaponPal, 36700);
                }
                break;
            }

            case CHICKEN_WEAPON__STATIC:
            {
                if (!RRRA) break;
                if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING))
                    weaponBits |= 512;
                if (*weaponFrame)
                {
                    static int frames[] = { 0,1,1,2,2,3,2,3,2,3,2,2,2,2,2,2,2,2,2,4,4,4,4,5,5,5,5,6,6,6,6,6,6,7,7,7,7,7,7 };
                    int frame = frames[*weaponFrame];
                    if (frame == 2 || frame == 3)
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 200 - halfLookAng, weaponY + 250 - weaponYOffset,
                                               TILE_RPGGUN2 + frame, weaponShade, weaponBits, weaponPal, 36700);
                    }
                    else if (frame == 1)
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 200 - halfLookAng, weaponY + 250 - weaponYOffset,
                                               TILE_RPGGUN2 + frame, 0, weaponBits, weaponPal, 36700);
                    }
                    else
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 210 - halfLookAng, weaponY + 255 - weaponYOffset,
                                               TILE_RPGGUN2 + frame, weaponShade, weaponBits, weaponPal, 36700);
                    }
                }
                else
                {
                    if (!g_netServer && ud.multimode < 2)
                    {
                        if (chickenphase)
                        {
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 210 - halfLookAng, weaponY + 222 - weaponYOffset,
                                                   TILE_RPGGUN2+7, weaponShade, weaponBits, weaponPal, 36700);
                        }
                        else if ((krand2() & 15) == 5)
                        {
                            A_PlaySound(327, pPlayer->i);
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 210 - halfLookAng, weaponY + 222 - weaponYOffset,
                                                   TILE_RPGGUN2+7, weaponShade, weaponBits, weaponPal, 36700);
                            chickenphase = 6;
                        }
                        else
                        {
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 210 - halfLookAng, weaponY + 225 - weaponYOffset,
                                                   TILE_RPGGUN2, weaponShade, weaponBits, weaponPal, 36700);
                        }

                    }
                    else
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 210 - halfLookAng, weaponY + 225 - weaponYOffset,
                                               TILE_RPGGUN2, weaponShade, weaponBits, weaponPal, 36700);
                    }
                }
                break;
            }

            case SHOTGUN_WEAPON__STATIC:
                weaponX -= 8;
                {
                    static int kb_frames3[] = { 0,0,1,1,2,2,5,5,6,6,7,7,8,8,0,0,0,0,0,0,0 };
                    static int kb_frames2[] = { 0,0,3,3,4,4,5,5,6,6,7,7,8,8,0,0,20,20,21,21,21,21,20,20,20,20,0,0 };
                    static int kb_frames[] = { 0,0,1,1,2,2,3,3,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8,0,0,20,20,21,21,21,21,20,20,20,20,0,0 };
                    static int kb_ox[] = { 300,300,300,300,300,330,320,310,305,306,302 };
                    static int kb_oy[] = { 315,300,302,305,302,302,303,306,302,404,384 };
                    if (pPlayer->shotgun_state[1])
                    {
                        if (*weaponFrame < 26)
                        {
                            int frm = kb_frames[*weaponFrame];
                            if (frm == 3 || frm == 4)
                                weaponShade = 0;
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 52 - halfLookAng + (kb_ox[frm] >> 1),
                                weaponY + kb_oy[frm] - 64 - weaponYOffset, TILE_SHOTGUN + frm, weaponShade, weaponBits, weaponPal, 32768);
                        }
                        else
                        {
                            int frm;
                            if (kb_frames[*weaponFrame] > 0)
                                frm = kb_frames[(*weaponFrame) - 11];
                            else
                                frm = kb_frames[*weaponFrame];
                            weaponX += (kb_ox[frm] >> 1) - 12;
                            weaponY += kb_oy[frm] - 64;
                            switch (*weaponFrame)
                            {
                            case 23:
                                weaponY += 60;
                                break;
                            case 24:
                                weaponY += 30;
                                break;
                            }
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 64 - halfLookAng, weaponY - weaponYOffset,
                                TILE_SHOTGUN + kb_frames[*weaponFrame], weaponShade, weaponBits, weaponPal, 32768);
                            if (kb_frames[*weaponFrame] == 21)
                                G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 96 - halfLookAng, weaponY - weaponYOffset,
                                    TILE_SHOTGUNSHELLS, weaponShade, weaponBits, weaponPal, 32768);
                        }
                    }
                    else
                    {
                        if ((*weaponFrame) < 16)
                        {
                            if (pPlayer->shotgun_state[0])
                            {
                                int frm = kb_frames2[*weaponFrame];
                                if (frm == 3 || frm == 4)
                                    weaponShade = 0;
                                G_DrawWeaponTileWithID(currentWeapon, weaponX + 52 - halfLookAng + (kb_ox[frm] >> 1),
                                    weaponY + kb_oy[frm] - 64 - weaponYOffset, TILE_SHOTGUN + frm, weaponShade, weaponBits, weaponPal, 32768);
                            }
                            else
                            {
                                int frm = kb_frames3[*weaponFrame];
                                if (frm == 1 || frm == 2)
                                    weaponShade = 0;
                                G_DrawWeaponTileWithID(currentWeapon, weaponX + 52 - halfLookAng + (kb_ox[frm] >> 1),
                                    weaponY + kb_oy[frm] - 64 - weaponYOffset, TILE_SHOTGUN + frm, weaponShade, weaponBits, weaponPal, 32768);
                            }
                        }
                        else if (pPlayer->shotgun_state[0])
                        {
                            int frm;
                            if (kb_frames2[*weaponFrame] > 0)
                                frm = kb_frames2[(*weaponFrame) - 11];
                            else
                                frm = kb_frames2[*weaponFrame];
                            weaponX += (kb_ox[frm] >> 1) - 12;
                            weaponY += kb_oy[frm] - 64;
                            switch (*weaponFrame)
                            {
                            case 23:
                                weaponY += 60;
                                break;
                            case 24:
                                weaponY += 30;
                                break;
                            }
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 64 - halfLookAng, weaponY - weaponYOffset,
                                TILE_SHOTGUN + kb_frames2[*weaponFrame], weaponShade, weaponBits, weaponPal, 32768);
                            if (kb_frames2[*weaponFrame] == 21)
                                G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 96 - halfLookAng, weaponY - weaponYOffset,
                                    TILE_SHOTGUNSHELLS, weaponShade, weaponBits, weaponPal, 32768);
                        }
                    }
                }
                break;

            case CHAINGUN_WEAPON__STATIC:
                if (*weaponFrame > 0)
                {
                    weaponYOffset -= sintable[(*weaponFrame)<<7]>>12;

                    if (doAnim)
                        weaponX += 1-(rand()&3);
                }

                switch (*weaponFrame)
                {
                case 0:
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 178 - (pPlayer->look_ang >> 1) + 30, weaponY + 233 - weaponYOffset + 5,
                                            TILE_CHAINGUN, weaponShade, weaponBits, weaponPal, 32768);
                    break;

                default:
                    if (*weaponFrame < 8)
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 178 - (pPlayer->look_ang >> 1) + 30, weaponY + 233 - weaponYOffset + 5,
                                                TILE_CHAINGUN + 1, 0, weaponBits, weaponPal, 32768);
                    else
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 178 - (pPlayer->look_ang >> 1) + 30, weaponY + 233 - weaponYOffset + 5,
                                                TILE_CHAINGUN + 2, weaponShade, weaponBits, weaponPal, 32768);

                    break;
                }
                break;

            case PISTOL_WEAPON__STATIC:
                if ((*weaponFrame) < 22)
                {
                    static int frames[] = { 0,0,1,1,2,2,3,3,4,4,6,6,6,6,5,5,4,4,3,3,0,0 };
                    static int offsetX[] = { 194,190,185,208,215,215,216,216,201,170 };
                    static int offsetY[] = { 256,249,248,238,228,218,208,256,245,258 };
                    int frame = frames[*weaponFrame];

                    if (frame)
                        weaponShade = 0;

                    G_DrawWeaponTileWithID(currentWeapon, offsetX[frame] - 12 + weaponX - (pPlayer->look_ang >> 1), weaponY + offsetY[frame] - weaponYOffset,
                                            TILE_FIRSTGUN + frame, weaponShade, weaponBits, weaponPal, 36700);

                    break;
                }
                else
                {
                    static int frames[] = { 0,0,1,1,2,2,2,2,2,2,2,2,2,2,2,1,1,0,0 };
                    static int offsetX[] = { 244,244,244 };
                    static int offsetY[] = { 256,249,248 };
                    int dx, dy;
                    int frame = frames[(*weaponFrame) - 22];
                    switch (*weaponFrame)
                    {
                        case 28:
                            dy = 10;
                            dx = 5;
                            break;
                        case 29:
                            dy = 20;
                            dx = 10;
                            break;
                        case 30:
                            dy = 30;
                            dx = 15;
                            break;
                        case 31:
                            dy = 40;
                            dx = 20;
                            break;
                        case 32:
                            dy = 50;
                            dx = 25;
                            break;
                        case 33:
                            dy = 40;
                            dx = 20;
                            break;
                        case 34:
                            dy = 30;
                            dx = 15;
                            break;
                        case 35:
                            dy = 20;
                            dx = 10;
                            break;
                        case 36:
                            dy = 10;
                            dx = 5;
                            break;
                        default:
                            dy = 0;
                            dx = 0;
                            break;
                    }

                    G_DrawWeaponTileWithID(currentWeapon, weaponX + offsetX[frame] - 12 - dx - halfLookAng, weaponY + offsetY[frame] - weaponYOffset + dy,
                                            TILE_FIRSTGUNRELOAD + frame, weaponShade, weaponBits, weaponPal, 36700);
                }
                break;

            case HANDBOMB_WEAPON__STATIC:
                weaponYOffset -= 9 * (*weaponFrame);
                G_DrawWeaponTileWithID(currentWeapon, weaponX + 190 - halfLookAng, weaponY + 260 - weaponYOffset,
                                        TILE_HANDTHROW, weaponShade, weaponBits, weaponPal, 36700);               
                break;

            case HANDREMOTE_WEAPON__STATIC:
                if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING))
                    weaponBits |= 512;

                if(*weaponFrame < 20)
                {
                    static uint8_t remoteFrames[] = { 1,1,1,1,1,2,2,2,2,3,3,3,4,4,4,5,5,5,5,5,6,6,6 };

                    if (*weaponFrame >= ARRAY_SIZE(remoteFrames))
                        break;

                    if (*weaponFrame < 5)
                    {
                        G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 265 - halfLookAng, weaponY + 174 - weaponYOffset + pPlayer->detonate_count,
                                                TILE_RRTILE1752, 0, weaponBits, weaponPal, 36700);
                    }
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 290 - halfLookAng, weaponY + 238 - weaponYOffset,
                                            TILE_HANDTHROW + remoteFrames[(*weaponFrame)], 0, weaponBits, weaponPal, 36700);
                }
                break;

            case DEVISTATOR_WEAPON__STATIC:
                if (*weaponFrame)
                    G_DrawWeaponTileWithID(currentWeapon, 150 + (weaponX >> 1) - halfLookAng, 266 + (weaponY >> 1) - weaponYOffset,
                                            TILE_DEVISTATOR, 0, weaponBits, weaponPal, 47040);
                else
                    G_DrawWeaponTileWithID(currentWeapon, 150 + (weaponX >> 1) - halfLookAng, 266 + (weaponY >> 1) - weaponYOffset,
                                            TILE_DEVISTATOR + 1, weaponShade, weaponBits, weaponPal, 47040);
                break;

            case FREEZE_WEAPON__STATIC:
                if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING))
                    weaponBits |= 512;

                if ((*weaponFrame) > 0)
                {
                    static uint8_t freezerFrames[] = { 0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 260 - (pPlayer->look_ang >> 1), weaponY + 215 - weaponYOffset,
                                            TILE_FREEZE + freezerFrames[*weaponFrame], -32, weaponBits, weaponPal, 32768);
                }
                else
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 260 - (pPlayer->look_ang >> 1), weaponY + 215 - weaponYOffset,
                                            TILE_FREEZE, weaponShade, weaponBits, weaponPal, 32768);
                break;

            case GROW_WEAPON__STATIC:
            case SHRINKER_WEAPON__STATIC:
                weaponX += 28;
                weaponY += 18;

                if ((*weaponFrame) > 0)
                {
                    if (doAnim)
                    {
                        weaponX += rand() & 3;
                        weaponYOffset += (rand() & 3);
                    }

                    if (currentWeapon == GROW_WEAPON)
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 184 - halfLookAng, weaponY + 240 - weaponYOffset,
                                                TILE_GROWSPARK + ((*weaponFrame)&2), weaponShade, weaponBits, 0, 44040);
                    }
                    else
                    {
                        static int frames[] = { 1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,0,0,0,0,0,0,0,0,0,0 };
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 184 - halfLookAng, weaponY + 240 - weaponYOffset,
                            TILE_SHRINKER + frames[*weaponFrame], weaponShade, weaponBits, 0, 44040);
                    }
                }
                else
                {
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 188 - halfLookAng, weaponY + 240 - weaponYOffset,
                                            TILE_SHRINKER, weaponShade, weaponBits, weaponPal, 44040);
                }
                break;
            }
        }
        else
        {
            switch (DYNAMICWEAPONMAP(currentWeapon))
            {
            case KNEE_WEAPON__STATIC:
            {
                int const kneePal = P_GetKneePal(pPlayer, weaponPal);

                guniqhudid = currentWeapon;
                if (*weaponFrame < 5 || *weaponFrame > 9)
                    G_DrawTileScaled(weaponX + 220 - halfLookAng, weaponY + 250 - weaponYOffset, TILE_KNEE,
                                        weaponShade, weaponBits, kneePal);
                else
                    G_DrawTileScaled(weaponX + 160 - halfLookAng, weaponY + 214 - weaponYOffset, TILE_KNEE + 1,
                                        weaponShade, weaponBits, kneePal);
                guniqhudid = 0;
                break;
            }

            case TRIPBOMB_WEAPON__STATIC:
                weaponX += 8;
                weaponYOffset -= 10;

                if ((*weaponFrame) > 6)
                    weaponY += ((*weaponFrame) << 3);
                else if ((*weaponFrame) < 4)
                    G_DrawWeaponTileWithID(currentWeapon << 2, weaponX + 142 - halfLookAng,
                                            weaponY + 234 - weaponYOffset, TILE_HANDHOLDINGLASER + 3, weaponShade, weaponBits, weaponPal);

                G_DrawWeaponTileWithID(currentWeapon, weaponX + 130 - halfLookAng, weaponY + 249 - weaponYOffset,
                                        TILE_HANDHOLDINGLASER + ((*weaponFrame) >> 2), weaponShade, weaponBits, weaponPal);

                G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 152 - halfLookAng,
                                        weaponY + 249 - weaponYOffset, TILE_HANDHOLDINGLASER + ((*weaponFrame) >> 2), weaponShade, weaponBits | 4,
                                        weaponPal);
                break;

            case RPG_WEAPON__STATIC:
                weaponX -= sintable[(768 + ((*weaponFrame) << 7)) & 2047] >> 11;
                weaponYOffset += sintable[(768 + ((*weaponFrame) << 7)) & 2047] >> 11;

                if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING))
                    weaponBits |= 512;

                if (*weaponFrame > 0)
                {
                    int totalTime;
                    if (*weaponFrame < (WW2GI ? (totalTime = PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime)) : 8))
                        G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 164, (weaponY << 1) + 176 - weaponYOffset,
                            TILE_RPGGUN + ((*weaponFrame) >> 1), weaponShade, weaponBits, weaponPal);
                    else if (WW2GI)
                    {
                        totalTime = PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime);
                        int const reloadTime = PWEAPON(screenpeek, pPlayer->curr_weapon, Reload);

                        weaponYOffset -= (*weaponFrame < ((reloadTime - totalTime) / 2 + totalTime))
                                          ? 10 * ((*weaponFrame) - totalTime)   // down
                                          : 10 * (reloadTime - (*weaponFrame)); // up
                    }
                }

                G_DrawWeaponTileWithID(currentWeapon, weaponX + 164, (weaponY << 1) + 176 - weaponYOffset, TILE_RPGGUN, weaponShade,
                                        weaponBits, weaponPal);
                break;

            case SHOTGUN_WEAPON__STATIC:
                weaponX -= 8;

                if (WW2GI)
                {
                    int const totalTime  = PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime);
                    int const reloadTime = PWEAPON(screenpeek, pPlayer->curr_weapon, Reload);

                    if (*weaponFrame > 0)
                        weaponYOffset -= sintable[(*weaponFrame)<<7]>>12;

                    if (*weaponFrame > 0 && doAnim)
                        weaponX += 1-(rand()&3);

                    if (*weaponFrame == 0)
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 146 - halfLookAng, weaponY + 202 - weaponYOffset,
                                               TILE_SHOTGUN, weaponShade, weaponBits, weaponPal);
                    }
                    else if (*weaponFrame <= totalTime)
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 146 - halfLookAng, weaponY + 202 - weaponYOffset,
                                               TILE_SHOTGUN + 1, weaponShade, weaponBits, weaponPal);
                    }
                    // else we are in 'reload time'
                    else
                    {
                        weaponYOffset -= (*weaponFrame < ((reloadTime - totalTime) / 2 + totalTime))
                                         ? 10 * ((*weaponFrame) - totalTime)    // D
                                         : 10 * (reloadTime - (*weaponFrame));  // U

                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 146 - halfLookAng, weaponY + 202 - weaponYOffset,
                                               TILE_SHOTGUN, weaponShade, weaponBits, weaponPal);
                    }

                    break;
                }

                switch (*weaponFrame)
                {
                    case 1:
                    case 2:
                        G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 168 - halfLookAng, weaponY + 201 - weaponYOffset,
                                                TILE_SHOTGUN + 2, -128, weaponBits, weaponPal);
                        fallthrough__;
                    case 0:
                    case 6:
                    case 7:
                    case 8:
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 146 - halfLookAng, weaponY + 202 - weaponYOffset,
                                                TILE_SHOTGUN, weaponShade, weaponBits, weaponPal);
                        break;

                    case 3:
                    case 4:
                        weaponYOffset -= 40;
                        weaponX += 20;

                        G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 178 - halfLookAng, weaponY + 194 - weaponYOffset,
                                                TILE_SHOTGUN + 1 + ((*(weaponFrame)-1) >> 1), -128, weaponBits, weaponPal);
                        fallthrough__;
                    case 5:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 158 - halfLookAng, weaponY + 220 - weaponYOffset,
                                                TILE_SHOTGUN + 3, weaponShade, weaponBits, weaponPal);
                        break;

                    case 13:
                    case 14:
                    case 15:
                        G_DrawWeaponTileWithID(currentWeapon, 32 + weaponX + 166 - halfLookAng, weaponY + 210 - weaponYOffset,
                                                TILE_SHOTGUN + 4, weaponShade, weaponBits, weaponPal);
                        break;

                    case 16:
                    case 17:
                    case 18:
                    case 19:
                    case 24:
                    case 25:
                    case 26:
                    case 27:
                        G_DrawWeaponTileWithID(currentWeapon, 64 + weaponX + 170 - halfLookAng, weaponY + 196 - weaponYOffset,
                                                TILE_SHOTGUN + 5, weaponShade, weaponBits, weaponPal);
                        break;

                    case 20:
                    case 21:
                    case 22:
                    case 23:
                        G_DrawWeaponTileWithID(currentWeapon, 64 + weaponX + 176 - halfLookAng, weaponY + 196 - weaponYOffset,
                                                TILE_SHOTGUN + 6, weaponShade, weaponBits, weaponPal);
                        break;


                    case 28:
                    case 29:
                    case 30:
                        G_DrawWeaponTileWithID(currentWeapon, 32 + weaponX + 156 - halfLookAng, weaponY + 206 - weaponYOffset,
                                                TILE_SHOTGUN + 4, weaponShade, weaponBits, weaponPal);
                        break;
                }
                break;

            case CHAINGUN_WEAPON__STATIC:
                if (*weaponFrame > 0)
                {
                    weaponYOffset -= sintable[(*weaponFrame)<<7]>>12;

                    if (doAnim)
                        weaponX += 1-(rand()&3);
                }

                if (WW2GI)
                {
                    int const totalTime = PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime);
                    int const reloadTime = PWEAPON(screenpeek, pPlayer->curr_weapon, Reload);

                    if (*weaponFrame == 0)
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 178 - halfLookAng,weaponY+233-weaponYOffset,
                            TILE_CHAINGUN+1,weaponShade,weaponBits,weaponPal);
                    }
                    else if (*weaponFrame <= totalTime)
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 188 - halfLookAng,weaponY+243-weaponYOffset,
                            TILE_CHAINGUN+2,weaponShade,weaponBits,weaponPal);
                    }
                    // else we are in 'reload time'
                    // divide reload time into fifths..
                    // 1) move weapon up/right, hand on clip (TILE_CHAINGUN - 17)
                    // 2) move weapon up/right, hand removing clip (TILE_CHAINGUN - 18)
                    // 3) hold weapon up/right, hand removed clip (TILE_CHAINGUN - 19)
                    // 4) hold weapon up/right, hand inserting clip (TILE_CHAINGUN - 18)
                    // 5) move weapon down/left, clip inserted (TILE_CHAINGUN - 17)
                    else
                    {
                        int iFifths = (reloadTime - totalTime) / 5;
                        if (iFifths < 1)
                            iFifths = 1;

                        if (*weaponFrame < iFifths + totalTime)
                        {
                            // first segment
                            int const weaponOffset = 80 - 10 * (totalTime + iFifths - (*weaponFrame));
                            weaponYOffset += weaponOffset;
                            weaponX += weaponOffset;
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 168 - halfLookAng, weaponY + 260 - weaponYOffset, TILE_CHAINGUN - 17,
                                                   weaponShade, weaponBits, weaponPal);
                        }
                        else if (*weaponFrame < (iFifths * 2 + totalTime))
                        {
                            // second segment
                            weaponYOffset += 80; // D
                            weaponX += 80;
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 168 - halfLookAng, weaponY + 260 - weaponYOffset, TILE_CHAINGUN - 18,
                                                   weaponShade, weaponBits, weaponPal);
                        }
                        else if (*weaponFrame < (iFifths * 3 + totalTime))
                        {
                            // third segment
                            // up
                            weaponYOffset += 80;
                            weaponX += 80;
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 168 - halfLookAng, weaponY + 260 - weaponYOffset, TILE_CHAINGUN - 19,
                                                   weaponShade, weaponBits, weaponPal);
                        }
                        else if (*weaponFrame < (iFifths * 4 + totalTime))
                        {
                            // fourth segment
                            // down
                            weaponYOffset += 80; // D
                            weaponX += 80;
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 168 - halfLookAng, weaponY + 260 - weaponYOffset, TILE_CHAINGUN - 18,
                                                   weaponShade, weaponBits, weaponPal);
                        }
                        else
                        {
                            // up and left
                            int const weaponOffset = 10 * (reloadTime - (*weaponFrame));
                            weaponYOffset += weaponOffset; // U
                            weaponX += weaponOffset;
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 168 - halfLookAng, weaponY + 260 - weaponYOffset, TILE_CHAINGUN - 17,
                                                   weaponShade, weaponBits, weaponPal);
                        }
                    }

                    break;
                }

                switch (*weaponFrame)
                {
                case 0:
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 178 - (pPlayer->look_ang >> 1), weaponY + 233 - weaponYOffset,
                                            TILE_CHAINGUN + 1, weaponShade, weaponBits, weaponPal);
                    break;

                default:
                    if (*weaponFrame > 4 && *weaponFrame < 12)
                    {
                        int randomOffset = doAnim ? rand()&7 : 0;
                        G_DrawWeaponTileWithID(currentWeapon << 2, randomOffset + weaponX - 4 + 140 - (pPlayer->look_ang >> 1),
                                                randomOffset + weaponY - ((*weaponFrame) >> 1) + 208 - weaponYOffset,
                                                TILE_CHAINGUN + 5 + ((*weaponFrame - 4) / 5), weaponShade, weaponBits, weaponPal);
                        if (doAnim) randomOffset = rand()&7;
                        G_DrawWeaponTileWithID(currentWeapon << 2, randomOffset + weaponX - 4 + 184 - (pPlayer->look_ang >> 1),
                                                randomOffset + weaponY - ((*weaponFrame) >> 1) + 208 - weaponYOffset,
                                                TILE_CHAINGUN + 5 + ((*weaponFrame - 4) / 5), weaponShade, weaponBits, weaponPal);
                    }
                
                    if (*weaponFrame < 8)
                    {
                        int const randomOffset = doAnim ? rand()&7 : 0;
                        G_DrawWeaponTileWithID(currentWeapon << 2, randomOffset + weaponX - 4 + 162 - (pPlayer->look_ang >> 1),
                            randomOffset + weaponY - ((*weaponFrame) >> 1) + 208 - weaponYOffset,
                                                TILE_CHAINGUN + 5 + ((*weaponFrame - 2) / 5), weaponShade, weaponBits, weaponPal);
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 178 - (pPlayer->look_ang >> 1), weaponY + 233 - weaponYOffset,
                                                TILE_CHAINGUN + 1 + ((*weaponFrame) >> 1), weaponShade, weaponBits, weaponPal);
                    }
                    else
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 178 - (pPlayer->look_ang >> 1), weaponY + 233 - weaponYOffset,
                                                TILE_CHAINGUN + 1, weaponShade, weaponBits, weaponPal);

                    break;
                }

                G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 168 - (pPlayer->look_ang >> 1), weaponY + 260 - weaponYOffset,
                                        TILE_CHAINGUN, weaponShade, weaponBits, weaponPal);
                break;

            case PISTOL_WEAPON__STATIC:
                if ((*weaponFrame) < 5)
                {
                    static uint8_t pistolFrames[] = { 0, 1, 2 };
                    int pistolOffset = 195-12+weaponX;

                    if ((*weaponFrame) == 2)
                        pistolOffset -= 3;

                    G_DrawWeaponTileWithID(currentWeapon, (pistolOffset - (pPlayer->look_ang >> 1)), (weaponY + 244 - weaponYOffset),
                                            TILE_FIRSTGUN + pistolFrames[*weaponFrame > 2 ? 0 : *weaponFrame], weaponShade, 2,
                                            weaponPal);

                    break;
                }

                if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING) && DUKE)
                    weaponBits |= 512;

                if ((*weaponFrame) < 10)
                    G_DrawWeaponTileWithID(currentWeapon, 194 - (pPlayer->look_ang >> 1), weaponY + 230 - weaponYOffset, TILE_FIRSTGUN + 4,
                                            weaponShade, weaponBits, weaponPal);
                else if ((*weaponFrame) < 15)
                {
                    G_DrawWeaponTileWithID(currentWeapon << 1, 244 - ((*weaponFrame) << 3) - (pPlayer->look_ang >> 1),
                                            weaponY + 130 - weaponYOffset + ((*weaponFrame) << 4), TILE_FIRSTGUN + 6, weaponShade,
                                            weaponBits, weaponPal);
                    G_DrawWeaponTileWithID(currentWeapon, 224 - (pPlayer->look_ang >> 1), weaponY + 220 - weaponYOffset, TILE_FIRSTGUN + 5,
                                            weaponShade, weaponBits, weaponPal);
                }
                else if ((*weaponFrame) < 20)
                {
                    G_DrawWeaponTileWithID(currentWeapon << 1, 124 + ((*weaponFrame) << 1) - (pPlayer->look_ang >> 1),
                                            weaponY + 430 - weaponYOffset - ((*weaponFrame) << 3), TILE_FIRSTGUN + 6, weaponShade,
                                            weaponBits, weaponPal);
                    G_DrawWeaponTileWithID(currentWeapon, 224 - (pPlayer->look_ang >> 1), weaponY + 220 - weaponYOffset, TILE_FIRSTGUN + 5,
                                            weaponShade, weaponBits, weaponPal);
                }

                else if ((*weaponFrame) < (WW2GI ? PWEAPON(screenpeek, PISTOL_WEAPON, Reload) - 12 : (NAM ? 38 : 23)))
                {
                    G_DrawWeaponTileWithID(currentWeapon << 2, 184 - (pPlayer->look_ang >> 1), weaponY + 235 - weaponYOffset,
                                            TILE_FIRSTGUN + 8, weaponShade, weaponBits, weaponPal);
                    G_DrawWeaponTileWithID(currentWeapon, 224 - (pPlayer->look_ang >> 1), weaponY + 210 - weaponYOffset, TILE_FIRSTGUN + 5,
                                            weaponShade, weaponBits, weaponPal);
                }
                else if ((*weaponFrame) < (WW2GI ? PWEAPON(screenpeek, PISTOL_WEAPON, Reload) - 6 : (NAM ? 44 : 25)))
                {
                    G_DrawWeaponTileWithID(currentWeapon << 2, 164 - (pPlayer->look_ang >> 1), weaponY + 245 - weaponYOffset,
                                            TILE_FIRSTGUN + 8, weaponShade, weaponBits, weaponPal);
                    G_DrawWeaponTileWithID(currentWeapon, 224 - (pPlayer->look_ang >> 1), weaponY + 220 - weaponYOffset, TILE_FIRSTGUN + 5,
                                            weaponShade, weaponBits, weaponPal);
                }
                else if ((*weaponFrame) < (WW2GI ? PWEAPON(screenpeek, PISTOL_WEAPON, Reload) : (NAM ? 50 : 27)))
                    G_DrawWeaponTileWithID(currentWeapon, 194 - (pPlayer->look_ang >> 1), weaponY + 235 - weaponYOffset, TILE_FIRSTGUN + 5,
                                            weaponShade, weaponBits, weaponPal);

                break;

            case HANDBOMB_WEAPON__STATIC:
                {
                    static uint8_t pipebombFrames [] = { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2 };

                    if (*weaponFrame >= ARRAY_SIZE(pipebombFrames))
                        break;

                    if (WW2GI && *weaponFrame >= PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime))
                        break;

                    if (*weaponFrame)
                    {
                        if (WW2GI)
                        {
                            int const fireDelay = PWEAPON(screenpeek, pPlayer->curr_weapon, FireDelay);
                            int const totalTime = PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime);

                            if (*weaponFrame <= fireDelay)
                            {
                                // it holds here
                                weaponYOffset -= 5 * (*weaponFrame);  // D
                            }
                            else if (*weaponFrame < ((totalTime - fireDelay) / 2 + fireDelay))
                            {
                                // up and left
                                int const weaponOffset = (*weaponFrame) - fireDelay;
                                weaponYOffset += 10 * weaponOffset;  // U
                                weaponX += 80 * weaponOffset;
                            }
                            else if (*weaponFrame < totalTime)
                            {
                                // start high
                                weaponYOffset += 240;
                                weaponYOffset -= 12 * ((*weaponFrame) - fireDelay);  // D
                                // move left
                                weaponX += 90 - 5 * (totalTime - (*weaponFrame));
                            }
                        }
                        else
                        {
                            if (*weaponFrame < 7)       weaponYOffset -= 10 * (*weaponFrame);  // D
                            else if (*weaponFrame < 12) weaponYOffset += 20 * ((*weaponFrame) - 10);  // U
                            else if (*weaponFrame < 20) weaponYOffset -= 9  * ((*weaponFrame) - 14);  // D

                            weaponYOffset += 10;
                        }
                    }

                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 190 - halfLookAng, weaponY + 260 - weaponYOffset,
                                            TILE_HANDTHROW + pipebombFrames[(*weaponFrame)], weaponShade, weaponBits, weaponPal);
                }
                break;

            case HANDREMOTE_WEAPON__STATIC:
                {
                    static uint8_t remoteFrames[] = { 0, 1, 1, 2, 1, 1, 0, 0, 0, 0, 0 };

                    if (*weaponFrame >= ARRAY_SIZE(remoteFrames))
                        break;

                    weaponX = -48;
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 150 - halfLookAng, weaponY + 258 - weaponYOffset,
                                            TILE_HANDREMOTE + remoteFrames[(*weaponFrame)], weaponShade, weaponBits, weaponPal);
                }
                break;

            case DEVISTATOR_WEAPON__STATIC:
                if (WW2GI)
                {
                    if (*weaponFrame)
                    {
                        int32_t const totalTime = PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime);
                        int32_t const reloadTime = PWEAPON(screenpeek, pPlayer->curr_weapon, Reload);

                        if (*weaponFrame < totalTime)
                        {
                            int const tileOffset = ksgn((*weaponFrame) >> 2);

                            if (pPlayer->ammo_amount[pPlayer->curr_weapon] & 1)
                            {
                                G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 30 - halfLookAng, weaponY + 240 - weaponYOffset,
                                                       TILE_DEVISTATOR, weaponShade, weaponBits | 4, weaponPal);
                                G_DrawWeaponTileWithID(currentWeapon, weaponX + 268 - halfLookAng, weaponY + 238 - weaponYOffset,
                                                       TILE_DEVISTATOR + tileOffset, -32, weaponBits, weaponPal);
                            }
                            else
                            {
                                G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 30 - halfLookAng, weaponY + 240 - weaponYOffset,
                                                       TILE_DEVISTATOR + tileOffset, -32, weaponBits | 4, weaponPal);
                                G_DrawWeaponTileWithID(currentWeapon, weaponX + 268 - halfLookAng, weaponY + 238 - weaponYOffset, TILE_DEVISTATOR,
                                                       weaponShade, weaponBits, weaponPal);
                            }
                        }
                        // else we are in 'reload time'
                        else
                        {
                            weaponYOffset -= (*weaponFrame < ((reloadTime - totalTime) / 2 + totalTime))
                                             ? 10 * ((*weaponFrame) - totalTime)
                                             : 10 * (reloadTime - (*weaponFrame));

                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 268 - halfLookAng, weaponY + 238 - weaponYOffset, TILE_DEVISTATOR,
                                                   weaponShade, weaponBits, weaponPal);
                            G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 30 - halfLookAng, weaponY + 240 - weaponYOffset, TILE_DEVISTATOR,
                                                   weaponShade, weaponBits | 4, weaponPal);
                        }
                    }
                    else
                    {
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 268 - halfLookAng, weaponY + 238 - weaponYOffset, TILE_DEVISTATOR,
                                               weaponShade, weaponBits, weaponPal);
                        G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 30 - halfLookAng, weaponY + 240 - weaponYOffset, TILE_DEVISTATOR,
                                               weaponShade, weaponBits | 4, weaponPal);
                    }
                    break;
                }

                if (*weaponFrame > 0)
                {
                    static uint8_t const devastatorFrames[] = { 0, 4, 12, 24, 12, 4, 0 };

                    if (*weaponFrame >= ARRAY_SIZE(devastatorFrames))
                        break;

                    int const tileOffset = ksgn((*weaponFrame) >> 2);

                    if (pPlayer->hbomb_hold_delay)
                    {
                        G_DrawWeaponTileWithID(currentWeapon, (devastatorFrames[*weaponFrame] >> 1) + weaponX + 268 - halfLookAng,
                                                devastatorFrames[*weaponFrame] + weaponY + 238 - weaponYOffset,
                                                TILE_DEVISTATOR + tileOffset, -32, weaponBits, weaponPal);
                        G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 30 - halfLookAng, weaponY + 240 - weaponYOffset, TILE_DEVISTATOR,
                                                weaponShade, weaponBits | 4, weaponPal);
                    }
                    else
                    {
                        G_DrawWeaponTileWithID(currentWeapon << 1, -(devastatorFrames[*weaponFrame] >> 1) + weaponX + 30 - halfLookAng,
                                                devastatorFrames[*weaponFrame] + weaponY + 240 - weaponYOffset,
                                                TILE_DEVISTATOR + tileOffset, -32, weaponBits | 4, weaponPal);
                        G_DrawWeaponTileWithID(currentWeapon, weaponX + 268 - halfLookAng, weaponY + 238 - weaponYOffset, TILE_DEVISTATOR,
                                                weaponShade, weaponBits, weaponPal);
                    }
                }
                else
                {
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 268 - halfLookAng, weaponY + 238 - weaponYOffset, TILE_DEVISTATOR, weaponShade,
                                            weaponBits, weaponPal);
                    G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 30 - halfLookAng, weaponY + 240 - weaponYOffset, TILE_DEVISTATOR,
                                            weaponShade, weaponBits | 4, weaponPal);
                }
                break;

            case FREEZE_WEAPON__STATIC:
                if (!(duke3d_globalflags & DUKE3D_NO_WIDESCREEN_PINNING) && DUKE)
                    weaponBits |= 512;

                if ((*weaponFrame) > 0)
                {
                    static uint8_t freezerFrames[] = { 0, 0, 1, 1, 2, 2 };

                    if (*weaponFrame % 6 >= ARRAY_SIZE(freezerFrames))
                        break;

                    if (doAnim)
                    {
                        weaponX += rand() & 3;
                        weaponY += rand() & 3;
                    }
                    weaponYOffset -= 16;
                    G_DrawWeaponTileWithID(currentWeapon << 1, weaponX + 210 - (pPlayer->look_ang >> 1), weaponY + 261 - weaponYOffset,
                                            TILE_FREEZE + 2, -32, weaponBits, weaponPal);
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 210 - (pPlayer->look_ang >> 1), weaponY + 235 - weaponYOffset,
                                            TILE_FREEZE + 3 + freezerFrames[*weaponFrame % 6], -32, weaponBits, weaponPal);
                }
                else
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 210 - (pPlayer->look_ang >> 1), weaponY + 261 - weaponYOffset,
                                            TILE_FREEZE, weaponShade, weaponBits, weaponPal);
                break;

            case GROW_WEAPON__STATIC:
            case SHRINKER_WEAPON__STATIC:
                weaponX += 28;
                weaponY += 18;

                if (WW2GI)
                {
                    if (*weaponFrame == 0)
                    {
                        // the 'at rest' display
                        if (currentWeapon == GROW_WEAPON)
                        {
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 188 - halfLookAng, weaponY + 240 - weaponYOffset, TILE_SHRINKER - 2,
                                                   weaponShade, weaponBits, weaponPal);
                            break;
                        }
                        else if (pPlayer->ammo_amount[currentWeapon] > 0)
                        {
                            G_DrawWeaponTileUnfadedWithID(currentWeapon << 1, weaponX + 184 - halfLookAng, weaponY + 240 - weaponYOffset, TILE_SHRINKER + 2,
                                                          16 - (sintable[pPlayer->random_club_frame & 2047] >> 10), weaponBits, 0);
                            G_DrawWeaponTileWithID(currentWeapon, weaponX + 188 - halfLookAng, weaponY + 240 - weaponYOffset, TILE_SHRINKER,
                                                   weaponShade, weaponBits, weaponPal);
                            break;
                        }
                    }
                    else
                    {
                        // the 'active' display.
                        if (doAnim)
                        {
                            weaponX += rand() & 3;
                            weaponYOffset += rand() & 3;
                        }

                        int const totalTime = PWEAPON(screenpeek, pPlayer->curr_weapon, TotalTime);
                        int const reloadTime = PWEAPON(screenpeek, pPlayer->curr_weapon, Reload);

                        if (*weaponFrame < totalTime)
                        {
                            if (*weaponFrame >= PWEAPON(screenpeek, pPlayer->curr_weapon, FireDelay))
                            {
                                // after fire time.
                                // lower weapon to reload cartridge (not clip)
                                weaponYOffset -= (currentWeapon == GROW_WEAPON ? 15 : 10) * (totalTime - (*weaponFrame));
                            }
                        }
                        // else we are in 'reload time'
                        else
                        {
                            weaponYOffset -= (*weaponFrame < ((reloadTime - totalTime) / 2 + totalTime))
                                             ? (currentWeapon == GROW_WEAPON ? 5 : 10) * ((*weaponFrame) - totalTime) // D
                                             : 10 * (reloadTime - (*weaponFrame)); // U
                        }
                    }

                    G_DrawWeaponTileUnfadedWithID(currentWeapon << 1, weaponX + 184 - halfLookAng, weaponY + 240 - weaponYOffset,
                                                  TILE_SHRINKER + 3 + ((*weaponFrame) & 3), -32, weaponBits, currentWeapon == GROW_WEAPON ? 2 : 0);

                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 188 - halfLookAng, weaponY + 240 - weaponYOffset,
                                           TILE_SHRINKER + (currentWeapon == GROW_WEAPON ? -1 : 1), weaponShade, weaponBits, weaponPal);

                    break;
                }

                if ((*weaponFrame) > 0)
                {
                    if (doAnim)
                    {
                        weaponX += rand() & 3;
                        weaponYOffset += (rand() & 3);
                    }

                    G_DrawWeaponTileUnfadedWithID(currentWeapon << 1, weaponX + 184 - halfLookAng, weaponY + 240 - weaponYOffset,
                                                    TILE_SHRINKER + 3 + ((*weaponFrame) & 3), -32, weaponBits, currentWeapon == GROW_WEAPON ? 2 : 0);
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 188 - halfLookAng, weaponY + 240 - weaponYOffset,
                                            currentWeapon == GROW_WEAPON ? TILE_SHRINKER - 1 : TILE_SHRINKER + 1, weaponShade, weaponBits, weaponPal);
                }
                else
                {
                    G_DrawWeaponTileUnfadedWithID(currentWeapon << 1, weaponX + 184 - halfLookAng, weaponY + 240 - weaponYOffset,
                                                    TILE_SHRINKER + 2, 16 - (sintable[pPlayer->random_club_frame & 2047] >> 10), weaponBits,
                                                    currentWeapon == GROW_WEAPON ? 2 : 0);
                    G_DrawWeaponTileWithID(currentWeapon, weaponX + 188 - halfLookAng, weaponY + 240 - weaponYOffset,
                                            currentWeapon == GROW_WEAPON ? TILE_SHRINKER - 2 : TILE_SHRINKER, weaponShade, weaponBits, weaponPal);
                }
                break;
            }
        }
    }

enddisplayweapon:
    if (!RR)
        P_DisplaySpit();
}

#define TURBOTURNTIME (TICRATE/8) // 7
#define NORMALTURN    15
#define PREAMBLETURN  5
#define NORMALKEYMOVE 40
#define MAXVEL        ((NORMALKEYMOVE*2)+10)
#define MAXSVEL       ((NORMALKEYMOVE*2)+10)
#define MAXANGVEL     1024
#define MAXHORIZVEL   256

#define MOTOTURN      20
#define MAXVELMOTO    120

int32_t g_myAimStat = 0, g_oldAimStat = 0;
int32_t mouseyaxismode = -1;

enum inputlock_t
{
    IL_NOANGLE = 0x1,
    IL_NOHORIZ = 0x2,
    IL_NOMOVE  = 0x4,

    IL_NOTHING = IL_NOANGLE|IL_NOHORIZ|IL_NOMOVE,
};

static int P_CheckLockedMovement(int const playerNum)
{
    auto const pPlayer = g_player[playerNum].ps;

    if (pPlayer->on_crane >= 0)
        return IL_NOMOVE|IL_NOANGLE;

    if (pPlayer->newowner != -1)
        return IL_NOANGLE|IL_NOHORIZ;

    if (pPlayer->dead_flag || pPlayer->fist_incs || pPlayer->transporter_hold > 2 || pPlayer->hard_landing || pPlayer->access_incs > 0
        || pPlayer->knee_incs > 0
        || (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == TRIPBOMB_WEAPON && pPlayer->kickback_pic > 1
            && pPlayer->kickback_pic < PWEAPON(playerNum, pPlayer->curr_weapon, FireDelay)))
        return IL_NOTHING;

    return 0;
}

void P_GetInput(int const playerNum)
{
    auto      &thisPlayer = g_player[playerNum];
    auto const pPlayer    = thisPlayer.ps;
    ControlInfo info;

    if ((pPlayer->gm & (MODE_MENU|MODE_TYPE)) || (ud.pause_on && !inputState.GetKeyStatus(sc_Pause)))
    {
        if (!(pPlayer->gm&MODE_MENU))
            CONTROL_GetInput(&info);

        localInput = {};
        localInput.bits    = (((int32_t)g_gameQuit) << SK_GAMEQUIT);
        localInput.extbits |= (1 << 7);

        return;
    }

	D_ProcessEvents();

	bool mouseaim = in_mousemode || buttonMap.ButtonDown(gamefunc_Mouse_Aiming);

    if (numplayers == 1)
    {
        pPlayer->aim_mode = in_mousemode;
        pPlayer->auto_aim = cl_autoaim;
        pPlayer->weaponswitch = cl_weaponswitch;
    }


    CONTROL_GetInput(&info);


    // JBF: Run key behaviour is selectable
	
	int const     playerRunning    = G_CheckAutorun(buttonMap.ButtonDown(gamefunc_Run));
    int const     turnAmount       = playerRunning ? (NORMALTURN << 1) : NORMALTURN;
    constexpr int analogTurnAmount = (NORMALTURN << 1);
    int const     keyMove          = playerRunning ? (NORMALKEYMOVE << 1) : NORMALKEYMOVE;
    constexpr int analogExtent     = 32767; // KEEPINSYNC sdlayer.cpp

    input_t input {};

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        static int strafeyaw;

        input.svel = -(info.mousex + strafeyaw) >> 3;
        strafeyaw  = (info.mousex + strafeyaw) % 8;

        input.svel -= info.dyaw * keyMove / analogExtent;
    }
    else
    {
        input.q16avel = fix16_sadd(input.q16avel, fix16_sdiv(fix16_from_int(info.mousex), F16(32)));
        input.q16avel = fix16_sadd(input.q16avel, fix16_from_int(info.dyaw / analogExtent * (analogTurnAmount << 1)));
    }

    if (mouseaim)
        input.q16horz = fix16_sadd(input.q16horz, fix16_sdiv(fix16_from_int(info.mousey), F16(64)));
    else
        input.fvel = -(info.mousey >> 3);

    if (!in_mouseflip) input.q16horz = -input.q16horz;

    input.q16horz = fix16_ssub(input.q16horz, fix16_from_int(info.dpitch * analogTurnAmount / analogExtent));
    input.svel -= info.dx * keyMove / analogExtent;
    input.fvel -= info.dz * keyMove / analogExtent;

    static double lastInputTicks;
    auto const    currentHiTicks    = timerGetHiTicks();
    double const  elapsedInputTicks = currentHiTicks - lastInputTicks;

    lastInputTicks = currentHiTicks;

    auto scaleAdjustmentToInterval = [=](double x) { return x * REALGAMETICSPERSEC / (1000.0 / elapsedInputTicks); };

    if (buttonMap.ButtonDown(gamefunc_Strafe))
    {
        if (!localInput.svel)
        {
            if (buttonMap.ButtonDown(gamefunc_Turn_Left) && !(pPlayer->movement_lock & 4) && !localInput.svel)
                input.svel = keyMove;

            if (buttonMap.ButtonDown(gamefunc_Turn_Right) && !(pPlayer->movement_lock & 8) && !localInput.svel)
                input.svel = -keyMove;
        }
    }
    else
    {
        static int32_t turnHeldTime;
        static int32_t lastInputClock;  // MED
        int32_t const  elapsedTics = (int32_t)totalclock - lastInputClock;

        lastInputClock = (int32_t) totalclock;

        if (buttonMap.ButtonDown(gamefunc_Turn_Left))
        {
            turnHeldTime += elapsedTics;
            input.q16avel = fix16_ssub(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval((turnHeldTime >= TURBOTURNTIME) ? (turnAmount << 1) : (PREAMBLETURN << 1))));
        }
        else if (buttonMap.ButtonDown(gamefunc_Turn_Right))
        {
            turnHeldTime += elapsedTics;
            input.q16avel = fix16_sadd(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval((turnHeldTime >= TURBOTURNTIME) ? (turnAmount << 1) : (PREAMBLETURN << 1))));
        }
        else
            turnHeldTime = 0;
    }

    if (localInput.svel < keyMove && localInput.svel > -keyMove)
    {
        if (buttonMap.ButtonDown(gamefunc_Strafe_Left) && !(pPlayer->movement_lock & 4))
            input.svel += keyMove;

        if (buttonMap.ButtonDown(gamefunc_Strafe_Right) && !(pPlayer->movement_lock & 8))
            input.svel += -keyMove;
    }

    if (localInput.fvel < keyMove && localInput.fvel > -keyMove)
    {
        if (RR)
        {
            /*if (buttonMap.ButtonDown(gamefunc_Quick_Kick))
            {
                localInput.bits |= buttonMap.ButtonDown(gamefunc_Move_Forward)<<SK_AIM_UP;
                localInput.bits |= buttonMap.ButtonDown(gamefunc_Move_Backward)<<SK_AIM_DOWN;
            }
            else*/
            {
                if (pPlayer->drink_amt >= 66 && pPlayer->drink_amt <= 87)
                {
                    if (buttonMap.ButtonDown(gamefunc_Move_Forward))
                    {
                        input.fvel += keyMove;
                        if (pPlayer->drink_amt & 1)
                            input.svel += keyMove;
                        else
                            input.svel -= keyMove;
                    }

                    if (buttonMap.ButtonDown(gamefunc_Move_Backward))
                    {
                        input.fvel += -keyMove;
                        if (pPlayer->drink_amt & 1)
                            input.svel -= keyMove;
                        else
                            input.svel += keyMove;
                    }
                }
                else
                {
                    if (buttonMap.ButtonDown(gamefunc_Move_Forward))
                        input.fvel += keyMove;

                    if (buttonMap.ButtonDown(gamefunc_Move_Backward))
                        input.fvel += -keyMove;
                }
            }
        }
        else
        {
            if (buttonMap.ButtonDown(gamefunc_Move_Forward) && !(pPlayer->movement_lock & 1))
                input.fvel += keyMove;

            if (buttonMap.ButtonDown(gamefunc_Move_Backward) && !(pPlayer->movement_lock & 2))
                input.fvel += -keyMove;
        }
    }

    int weaponSelection;

    for (weaponSelection = gamefunc_Weapon_10; weaponSelection >= gamefunc_Weapon_1; --weaponSelection)
    {
        if (buttonMap.ButtonDown(weaponSelection))
        {
            weaponSelection -= (gamefunc_Weapon_1 - 1);
            break;
        }
    }

    if (buttonMap.ButtonDown(gamefunc_Last_Weapon))
        weaponSelection = 14;
    else if (buttonMap.ButtonDown(gamefunc_Alt_Weapon))
        weaponSelection = 13;
    else if (buttonMap.ButtonPressed(gamefunc_Next_Weapon) || (buttonMap.ButtonDown(gamefunc_Dpad_Select) && input.fvel > 0))
    {
        weaponSelection = 12;
        buttonMap.ClearButton(gamefunc_Next_Weapon);
    }
    else if (buttonMap.ButtonPressed(gamefunc_Previous_Weapon) || (buttonMap.ButtonDown(gamefunc_Dpad_Select) && input.fvel < 0))
    {
        weaponSelection = 11;
        buttonMap.ClearButton(gamefunc_Previous_Weapon);
    }
    else if (weaponSelection == gamefunc_Weapon_1-1)
        weaponSelection = 0;

    if ((localInput.bits & 0xf00) == 0)
        localInput.bits |= (weaponSelection << SK_WEAPON_BITS);

    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Fire) << SK_FIRE);
    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Open) << SK_OPEN);

    int const sectorLotag = pPlayer->cursectnum != -1 ? sector[pPlayer->cursectnum].lotag : 0;
    int const crouchable = sectorLotag != 2 && (sectorLotag != 1 || pPlayer->spritebridge);

    if (buttonMap.ButtonDown(gamefunc_Toggle_Crouch))
    {
        pPlayer->crouch_toggle = !pPlayer->crouch_toggle && crouchable;

        if (crouchable)
            buttonMap.ClearButton(gamefunc_Toggle_Crouch);
    }

    if (buttonMap.ButtonDown(gamefunc_Crouch) || buttonMap.ButtonDown(gamefunc_Jump) || pPlayer->jetpack_on || (!crouchable && pPlayer->on_ground))
        pPlayer->crouch_toggle = 0;

    int const crouching = buttonMap.ButtonDown(gamefunc_Crouch) || buttonMap.ButtonDown(gamefunc_Toggle_Crouch) || pPlayer->crouch_toggle;

    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Jump) << SK_JUMP) | (crouching << SK_CROUCH);

    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Aim_Up) || (buttonMap.ButtonDown(gamefunc_Dpad_Aiming) && input.fvel > 0)) << SK_AIM_UP;
    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Aim_Down) || (buttonMap.ButtonDown(gamefunc_Dpad_Aiming) && input.fvel < 0)) << SK_AIM_DOWN;
    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Center_View) << SK_CENTER_VIEW);

    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Look_Left) << SK_LOOK_LEFT) | (buttonMap.ButtonDown(gamefunc_Look_Right) << SK_LOOK_RIGHT);
    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Look_Up) << SK_LOOK_UP) | (buttonMap.ButtonDown(gamefunc_Look_Down) << SK_LOOK_DOWN);

    localInput.bits |= (playerRunning << SK_RUN);

    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Inventory_Left) || (buttonMap.ButtonDown(gamefunc_Dpad_Select) && (input.svel > 0 || input.q16avel < 0))) << SK_INV_LEFT;
    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Inventory_Right) || (buttonMap.ButtonDown(gamefunc_Dpad_Select) && (input.svel < 0 || input.q16avel > 0))) << SK_INV_RIGHT;
    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Inventory) << SK_INVENTORY);

    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Steroids) << SK_STEROIDS) | (buttonMap.ButtonDown(gamefunc_NightVision) << SK_NIGHTVISION);
    localInput.bits |= (buttonMap.ButtonDown(gamefunc_MedKit) << SK_MEDKIT) | (buttonMap.ButtonDown(gamefunc_Holo_Duke) << SK_HOLODUKE);
    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Jetpack) << SK_JETPACK);

    localInput.bits |= buttonMap.ButtonDown(gamefunc_Holster_Weapon) << SK_HOLSTER;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_Quick_Kick) << SK_QUICK_KICK;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_TurnAround) << SK_TURNAROUND;

    localInput.bits |= (mouseaim << SK_AIMMODE);
    localInput.bits |= (g_gameQuit << SK_GAMEQUIT);
    localInput.bits |= inputState.GetKeyStatus(sc_Pause) << SK_PAUSE;
    //localInput.bits |= ((uint32_t)inputState.GetKeyStatus(sc_Escape)) << SK_ESCAPE; fixme.This needs to be done differently

    if (RR)
    {
        if (TEST_SYNC_KEY(localInput.bits, SK_CROUCH))
            localInput.bits &= ~(1 << SK_JUMP);
        if (pPlayer->drink_amt > 88)
            localInput.bits |= 1 << SK_LOOK_LEFT;
        if (pPlayer->drink_amt > 99)
            localInput.bits |= 1 << SK_LOOK_DOWN;
    }

    if (buttonMap.ButtonDown(gamefunc_Dpad_Select))
    {
        input.fvel = 0;
        input.svel = 0;
        input.q16avel = 0;
    }
    else if (buttonMap.ButtonDown(gamefunc_Dpad_Aiming))
        input.fvel = 0;

    localInput.extbits |= (buttonMap.ButtonDown(gamefunc_Move_Forward) || (input.fvel > 0));
    localInput.extbits |= (buttonMap.ButtonDown(gamefunc_Move_Backward) || (input.fvel < 0)) << 1;
    localInput.extbits |= (buttonMap.ButtonDown(gamefunc_Strafe_Left) || (input.svel > 0)) << 2;
    localInput.extbits |= (buttonMap.ButtonDown(gamefunc_Strafe_Right) || (input.svel < 0)) << 3;
    localInput.extbits |= buttonMap.ButtonDown(gamefunc_Turn_Left)<<4;
    localInput.extbits |= buttonMap.ButtonDown(gamefunc_Turn_Right)<<5;

    int const movementLocked = P_CheckLockedMovement(playerNum);

    if ((ud.scrollmode && ud.overhead_on) || (movementLocked & IL_NOTHING) == IL_NOTHING)
    {
        if (ud.scrollmode && ud.overhead_on)
        {
            ud.folfvel = input.fvel;
            ud.folavel = fix16_to_int(input.q16avel);
        }

        localInput.fvel = localInput.svel = 0;
        localInput.q16avel = localInput.q16horz = 0;
    }
    else
    {
        if (!(movementLocked & IL_NOMOVE))
        {
            localInput.fvel = clamp(localInput.fvel + input.fvel, -MAXVEL, MAXVEL);
            localInput.svel = clamp(localInput.svel + input.svel, -MAXSVEL, MAXSVEL);
        }

        if (!(movementLocked & IL_NOANGLE))
        {
            localInput.q16avel = fix16_sadd(localInput.q16avel, input.q16avel);
            pPlayer->q16ang    = fix16_sadd(pPlayer->q16ang, input.q16avel) & 0x7FFFFFF;
        }

        if (!(movementLocked & IL_NOHORIZ))
        {
            localInput.q16horz = fix16_clamp(fix16_sadd(localInput.q16horz, input.q16horz), F16(-MAXHORIZVEL), F16(MAXHORIZVEL));
            pPlayer->q16horiz  = fix16_clamp(fix16_sadd(pPlayer->q16horiz, input.q16horz), F16(HORIZ_MIN), F16(HORIZ_MAX));
        }
    }

    // A horiz diff of 128 equal 45 degrees, so we convert horiz to 1024 angle units

    if (thisPlayer.horizAngleAdjust)
    {
        float const horizAngle
        = atan2f(pPlayer->q16horiz - F16(100), F16(128)) * (512.f / fPI) + scaleAdjustmentToInterval(thisPlayer.horizAngleAdjust);
        pPlayer->q16horiz = F16(100) + Blrintf(F16(128) * tanf(horizAngle * (fPI / 512.f)));
    }
    else if (pPlayer->return_to_center > 0 || thisPlayer.horizRecenter)
    {
        pPlayer->q16horiz = fix16_sadd(pPlayer->q16horiz, fix16_from_dbl(scaleAdjustmentToInterval(fix16_to_dbl(fix16_from_dbl(200 / 3) - fix16_sdiv(pPlayer->q16horiz, F16(1.5))))));

        if ((!pPlayer->return_to_center && thisPlayer.horizRecenter) || (pPlayer->q16horiz >= F16(99.9) && pPlayer->q16horiz <= F16(100.1)))
        {
            pPlayer->q16horiz = F16(100);
            thisPlayer.horizRecenter = false;
        }

        if (pPlayer->q16horizoff >= F16(-0.1) && pPlayer->q16horizoff <= F16(0.1))
            pPlayer->q16horizoff = 0;
    }
 
    // calculates automatic view angle for playing without a mouse
    if (!pPlayer->aim_mode && pPlayer->on_ground && sectorLotag != ST_2_UNDERWATER && (sector[pPlayer->cursectnum].floorstat & 2))
    {
        // this is some kind of horse shit approximation of where the player is looking, I guess?
        vec2_t const adjustedPosition = { pPlayer->pos.x + (sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047] >> 5),
                                          pPlayer->pos.y + (sintable[fix16_to_int(pPlayer->q16ang) & 2047] >> 5) };
        int16_t currentSector = pPlayer->cursectnum;
 
        updatesector(adjustedPosition.x, adjustedPosition.y, &currentSector);
 
        if (currentSector >= 0)
        {
            int const slopeZ = getflorzofslope(pPlayer->cursectnum, adjustedPosition.x, adjustedPosition.y);
            if ((pPlayer->cursectnum == currentSector) || (klabs(getflorzofslope(currentSector, adjustedPosition.x, adjustedPosition.y) - slopeZ) <= ZOFFSET6))
                pPlayer->q16horizoff = fix16_sadd(pPlayer->q16horizoff, fix16_from_dbl(scaleAdjustmentToInterval(mulscale16(pPlayer->truefz - slopeZ, 160))));
        }
    }
 
    if (pPlayer->q16horizoff > 0)
    {
        pPlayer->q16horizoff = fix16_ssub(pPlayer->q16horizoff, fix16_from_dbl(scaleAdjustmentToInterval(fix16_to_dbl((pPlayer->q16horizoff >> 3) + fix16_one))));
        pPlayer->q16horizoff = fix16_max(pPlayer->q16horizoff, 0);
    }
    else if (pPlayer->q16horizoff < 0)
    {
        pPlayer->q16horizoff = fix16_sadd(pPlayer->q16horizoff, fix16_from_dbl(scaleAdjustmentToInterval(fix16_to_dbl((-pPlayer->q16horizoff >> 3) + fix16_one))));
        pPlayer->q16horizoff = fix16_min(pPlayer->q16horizoff, 0);
    }
 
    if (thisPlayer.horizSkew)
        pPlayer->q16horiz = fix16_sadd(pPlayer->q16horiz, fix16_from_dbl(scaleAdjustmentToInterval(fix16_to_dbl(thisPlayer.horizSkew))));
 
    pPlayer->q16horiz = fix16_clamp(pPlayer->q16horiz, F16(HORIZ_MIN), F16(HORIZ_MAX));
}

void P_GetInputMotorcycle(int playerNum)
{
    auto      &thisPlayer = g_player[playerNum];
    auto const pPlayer    = thisPlayer.ps;
    ControlInfo info;

    if ((pPlayer->gm & (MODE_MENU|MODE_TYPE)) || (ud.pause_on && !inputState.GetKeyStatus(sc_Pause)))
    {
        if (!(pPlayer->gm&MODE_MENU))
            CONTROL_GetInput(&info);

        localInput = {};
        localInput.bits    = (((int32_t)g_gameQuit) << SK_GAMEQUIT);
        localInput.extbits |= (1 << 7);

        return;
    }

    D_ProcessEvents();

	bool mouseaim = in_mousemode || buttonMap.ButtonDown(gamefunc_Mouse_Aiming);

    if (numplayers == 1)
    {
        pPlayer->aim_mode = in_mousemode;
        pPlayer->auto_aim = cl_autoaim;
        pPlayer->weaponswitch = cl_weaponswitch;
    }

    CONTROL_GetInput(&info);

    // JBF: Run key behaviour is selectable
    int const     playerRunning    = G_CheckAutorun(buttonMap.ButtonDown(gamefunc_Run));
    constexpr int analogTurnAmount = (NORMALTURN << 1);
    int const     keyMove          = playerRunning ? (NORMALKEYMOVE << 1) : NORMALKEYMOVE;
    constexpr int analogExtent     = 32767; // KEEPINSYNC sdlayer.cpp

    input_t input {};

    input.q16avel = fix16_sadd(input.q16avel, fix16_sdiv(fix16_from_int(info.mousex), F16(32)));
    input.q16avel = fix16_sadd(input.q16avel, fix16_from_int(info.dyaw / analogExtent * (analogTurnAmount << 1)));

    input.svel -= info.dx * keyMove / analogExtent;
    input.fvel -= info.dz * keyMove / analogExtent;

    static double lastInputTicks;
    auto const    currentHiTicks    = timerGetHiTicks();
    double const  elapsedInputTicks = currentHiTicks - lastInputTicks;

    lastInputTicks = currentHiTicks;

    auto scaleAdjustmentToInterval = [=](double x) { return x * REALGAMETICSPERSEC / (1000.0 / elapsedInputTicks); };

    pPlayer->crouch_toggle = 0;

    localInput.bits |= buttonMap.ButtonDown(gamefunc_Fire) << SK_FIRE;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_Steroids) << SK_STEROIDS;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_NightVision) << SK_NIGHTVISION;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_MedKit) << SK_MEDKIT;
    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Inventory_Left) ||
                 (buttonMap.ButtonDown(gamefunc_Dpad_Select) && (input.svel > 0 || input.q16avel < 0))) << SK_INV_LEFT;
    localInput.bits |= inputState.GetKeyStatus(sc_Pause) << SK_PAUSE;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_Holo_Duke) << SK_HOLODUKE;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_Jetpack) << SK_JETPACK;
    localInput.bits |= (g_gameQuit << SK_GAMEQUIT);
    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Inventory_Right) ||
                 (buttonMap.ButtonDown(gamefunc_Dpad_Select) && (input.svel < 0 || input.q16avel > 0))) << SK_INV_RIGHT;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_Open) << SK_OPEN;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_Inventory) << SK_INVENTORY;
    //localInput.bits |= ((uint32_t)inputState.GetKeyStatus(sc_Escape)) << SK_ESCAPE; fixme.This needs to be done differently

    if (buttonMap.ButtonDown(gamefunc_Dpad_Select))
    {
        input.fvel = 0;
        input.svel = 0;
        input.q16avel = 0;
    }

    if (buttonMap.ButtonDown(gamefunc_Dpad_Aiming))
        input.fvel = 0;
    
    localInput.extbits |= (buttonMap.ButtonDown(gamefunc_Move_Forward) || (input.fvel > 0));
    localInput.extbits |= (buttonMap.ButtonDown(gamefunc_Move_Backward) || (input.fvel < 0)) << 1;
    localInput.extbits |= (buttonMap.ButtonDown(gamefunc_Strafe_Left) || (input.svel > 0)) << 2;
    localInput.extbits |= (buttonMap.ButtonDown(gamefunc_Strafe_Right) || (input.svel < 0)) << 3;

    int turnAmount;
    int const turn = input.q16avel / 32;
    int turnLeft = buttonMap.ButtonDown(gamefunc_Turn_Left) || buttonMap.ButtonDown(gamefunc_Strafe_Left);
    int turnRight = buttonMap.ButtonDown(gamefunc_Turn_Right) || buttonMap.ButtonDown(gamefunc_Strafe_Right);
    int avelScale = F16((turnLeft || turnRight) ? 1 : 0);
    if (turn)
    {
        turnAmount = (MOTOTURN << 1);
        avelScale = fix16_max(avelScale, fix16_clamp(fix16_mul(turn, turn),0,F16(1)));
        if (turn < 0)
            turnLeft = 1;
        else if (turn > 0)
            turnRight = 1;
    }
    else
        turnAmount = MOTOTURN;

    input.svel = input.fvel = input.q16avel = 0;

    localInput.bits |= turnLeft << SK_AIM_DOWN;
    localInput.bits |= turnRight << SK_LOOK_LEFT;

    static int32_t turnHeldTime;
    static int32_t lastInputClock;  // MED
    int32_t const  elapsedTics = (int32_t)totalclock - lastInputClock;

    int const moveBack = buttonMap.ButtonDown(gamefunc_Move_Backward) && pPlayer->MotoSpeed <= 0;

    if (pPlayer->MotoSpeed == 0 || !pPlayer->on_ground)
    {
        if (turnLeft)
        {
            pPlayer->tilt_status--;
            if (pPlayer->tilt_status < -10)
                pPlayer->tilt_status = -10;
        }
        else if (turnRight)
        {
            pPlayer->tilt_status++;
            if (pPlayer->tilt_status > 10)
                pPlayer->tilt_status = 10;
        }
    }
    else
    {
        if (turnLeft || pPlayer->moto_drink < 0)
        {
            turnHeldTime += elapsedTics;
            pPlayer->tilt_status--;
            if (pPlayer->tilt_status < -10)
                pPlayer->tilt_status = -10;
            if (turnHeldTime >= TURBOTURNTIME && pPlayer->MotoSpeed > 0)
            {
                if (moveBack)
                    input.q16avel = fix16_sadd(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(turnAmount)));
                else
                    input.q16avel = fix16_ssub(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(turnAmount)));
            }
            else
            {
                if (moveBack)
                    input.q16avel = fix16_sadd(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(turnAmount / (8 / 3))));
                else
                    input.q16avel = fix16_ssub(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(turnAmount / (8 / 3))));
            }
        }
        else if (turnRight || pPlayer->moto_drink > 0)
        {
            turnHeldTime += elapsedTics;
            pPlayer->tilt_status++;
            if (pPlayer->tilt_status > 10)
                pPlayer->tilt_status = 10;
            if (turnHeldTime >= TURBOTURNTIME && pPlayer->MotoSpeed > 0)
            {
                if (moveBack)
                    input.q16avel = fix16_ssub(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(turnAmount)));
                else
                    input.q16avel = fix16_sadd(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(turnAmount)));
            }
            else
            {
                if (moveBack)
                    input.q16avel = fix16_ssub(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(turnAmount / (8 / 3))));
                else
                    input.q16avel = fix16_sadd(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(turnAmount / (8 / 3))));
            }
        }
        else
        {
            turnHeldTime = 0;

            if (pPlayer->tilt_status > 0)
                pPlayer->tilt_status--;
            else if (pPlayer->tilt_status < 0)
                pPlayer->tilt_status++;
        }
    }

    if (pPlayer->moto_underwater)
    {
        pPlayer->MotoSpeed = 0;
    }
    else
    {
        localInput.bits |= (buttonMap.ButtonDown(gamefunc_Move_Forward) || buttonMap.ButtonDown(gamefunc_Strafe)) << SK_JUMP;
        localInput.bits |= buttonMap.ButtonDown(gamefunc_Move_Backward) << SK_AIM_UP;
        localInput.bits |= buttonMap.ButtonDown(gamefunc_Run) << SK_CROUCH;
    }

    input.q16avel      = fix16_mul(input.q16avel, avelScale);
    localInput.q16avel = fix16_sadd(localInput.q16avel, input.q16avel);
    pPlayer->q16ang    = fix16_sadd(pPlayer->q16ang, input.q16avel) & 0x7FFFFFF;
    localInput.fvel    = clamp((input.fvel += pPlayer->MotoSpeed), -(MAXVELMOTO / 8), MAXVELMOTO);

    if (TEST_SYNC_KEY(localInput.bits, SK_JUMP))
    {
        localInput.bits |= 1;
    }
}

void P_GetInputBoat(int playerNum)
{
    auto      &thisPlayer = g_player[playerNum];
    auto const pPlayer    = thisPlayer.ps;
    ControlInfo info;

    if ((pPlayer->gm & (MODE_MENU|MODE_TYPE)) || (ud.pause_on && !inputState.GetKeyStatus(sc_Pause)))
    {
        if (!(pPlayer->gm&MODE_MENU))
            CONTROL_GetInput(&info);

        localInput = {};
        localInput.bits    = (((int32_t)g_gameQuit) << SK_GAMEQUIT);
        localInput.extbits |= (1 << 7);

        return;
    }

    D_ProcessEvents();

	bool mouseaim = in_mousemode || buttonMap.ButtonDown(gamefunc_Mouse_Aiming);

    if (numplayers == 1)
    {
        pPlayer->aim_mode = in_mousemode;
        pPlayer->auto_aim = cl_autoaim;
        pPlayer->weaponswitch = cl_weaponswitch;
    }

    CONTROL_GetInput(&info);

    // JBF: Run key behaviour is selectable
    int const     playerRunning    = G_CheckAutorun(buttonMap.ButtonDown(gamefunc_Run));
    constexpr int analogTurnAmount = (NORMALTURN << 1);
    int const     keyMove          = playerRunning ? (NORMALKEYMOVE << 1) : NORMALKEYMOVE;
    constexpr int analogExtent     = 32767; // KEEPINSYNC sdlayer.cpp

    input_t input {};

    input.q16avel = fix16_sadd(input.q16avel, fix16_sdiv(fix16_from_int(info.mousex), F16(32)));
    input.q16avel = fix16_sadd(input.q16avel, fix16_from_int(info.dyaw / analogExtent * (analogTurnAmount << 1)));

    input.svel -= info.dx * keyMove / analogExtent;
    input.fvel -= info.dz * keyMove / analogExtent;

    static double lastInputTicks;
    auto const    currentHiTicks    = timerGetHiTicks();
    double const  elapsedInputTicks = currentHiTicks - lastInputTicks;

    lastInputTicks = currentHiTicks;

    auto scaleAdjustmentToInterval = [=](double x) { return x * REALGAMETICSPERSEC / (1000.0 / elapsedInputTicks); };

    pPlayer->crouch_toggle = 0;

    localInput.bits |= buttonMap.ButtonDown(gamefunc_Fire) << SK_FIRE;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_Steroids) << SK_STEROIDS;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_NightVision) << SK_NIGHTVISION;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_MedKit) << SK_MEDKIT;
    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Inventory_Left) ||
                 (buttonMap.ButtonDown(gamefunc_Dpad_Select) && (input.svel > 0 || input.q16avel < 0))) << SK_INV_LEFT;
    localInput.bits |= inputState.GetKeyStatus(sc_Pause) << SK_PAUSE;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_Holo_Duke) << SK_HOLODUKE;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_Jetpack) << SK_JETPACK;
    localInput.bits |= (g_gameQuit << SK_GAMEQUIT);
    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Inventory_Right) ||
                 (buttonMap.ButtonDown(gamefunc_Dpad_Select) && (input.svel < 0 || input.q16avel > 0))) << SK_INV_RIGHT;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_Open) << SK_OPEN;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_Inventory) << SK_INVENTORY;
    //localInput.bits |= ((uint32_t)inputState.GetKeyStatus(sc_Escape)) << SK_ESCAPE; fixme.This needs to be done differently

    if (buttonMap.ButtonDown(gamefunc_Dpad_Select))
    {
        input.fvel = 0;
        input.svel = 0;
        input.q16avel = 0;
    }

    if (buttonMap.ButtonDown(gamefunc_Dpad_Aiming))
        input.fvel = 0;
    
    localInput.extbits |= (buttonMap.ButtonDown(gamefunc_Move_Forward) || (input.fvel > 0));
    localInput.extbits |= (buttonMap.ButtonDown(gamefunc_Move_Backward) || (input.fvel < 0)) << 1;
    localInput.extbits |= (buttonMap.ButtonDown(gamefunc_Strafe_Left) || (input.svel > 0)) << 2;
    localInput.extbits |= (buttonMap.ButtonDown(gamefunc_Strafe_Right) || (input.svel < 0)) << 3;

    int turnAmount;
    int const turn = input.q16avel / 32;
    int turnLeft = buttonMap.ButtonDown(gamefunc_Turn_Left) || buttonMap.ButtonDown(gamefunc_Strafe_Left);
    int turnRight = buttonMap.ButtonDown(gamefunc_Turn_Right) || buttonMap.ButtonDown(gamefunc_Strafe_Right);
    int avelScale = F16((turnLeft || turnRight) ? 1 : 0);
    if (turn)
    {
        turnAmount = (MOTOTURN << 1);
        avelScale = fix16_max(avelScale, fix16_clamp(fix16_mul(turn, turn),0,F16(1)));
        if (turn < 0)
            turnLeft = 1;
        else if (turn > 0)
            turnRight = 1;
    }
    else
        turnAmount = MOTOTURN;

    input.svel = input.fvel = input.q16avel = 0;

    localInput.bits |= (buttonMap.ButtonDown(gamefunc_Move_Forward) || buttonMap.ButtonDown(gamefunc_Strafe)) << SK_JUMP;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_Move_Backward) << SK_AIM_UP;
    localInput.bits |= buttonMap.ButtonDown(gamefunc_Run) << SK_CROUCH;

    localInput.bits |= turnLeft << SK_AIM_DOWN;
    localInput.bits |= turnRight << SK_LOOK_LEFT;

    static int32_t turnHeldTime;
    static int32_t lastInputClock;  // MED
    int32_t const  elapsedTics = (int32_t)totalclock - lastInputClock;

    if (pPlayer->MotoSpeed != 0)
    {
        if (turnLeft || pPlayer->moto_drink < 0)
        {
            turnHeldTime += elapsedTics;
            if (!pPlayer->not_on_water)
            {
                pPlayer->tilt_status--;
                if (pPlayer->tilt_status < -10)
                    pPlayer->tilt_status = -10;
                if (turnHeldTime >= TURBOTURNTIME)
                    input.q16avel = fix16_ssub(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(turnAmount)));
                else
                    input.q16avel = fix16_ssub(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(turnAmount / (10 / 3))));
            }
            else
                if (turnHeldTime >= TURBOTURNTIME)
                    input.q16avel = fix16_ssub(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(turnAmount / 3)));
                else
                    input.q16avel = fix16_ssub(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval((turnAmount / (10 / 3)) / 3)));
        }
        else if (turnRight || pPlayer->moto_drink > 0)
        {
            turnHeldTime += elapsedTics;
            if (!pPlayer->not_on_water)
            {
                pPlayer->tilt_status++;
                if (pPlayer->tilt_status > 10)
                    pPlayer->tilt_status = 10;
                if (turnHeldTime >= TURBOTURNTIME)
                    input.q16avel = fix16_sadd(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(turnAmount)));
                else
                    input.q16avel = fix16_sadd(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(turnAmount / (10 / 3))));
            }
            else
                if (turnHeldTime >= TURBOTURNTIME)
                    input.q16avel = fix16_sadd(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval(turnAmount / 3)));
                else
                    input.q16avel = fix16_sadd(input.q16avel, fix16_from_dbl(scaleAdjustmentToInterval((turnAmount / (10 / 3)) / 3)));
        }
        else if (!pPlayer->not_on_water)
        {
            turnHeldTime = 0;

            if (pPlayer->tilt_status > 0)
                pPlayer->tilt_status--;
            else if (pPlayer->tilt_status < 0)
                pPlayer->tilt_status++;
        }
    }
    else if (!pPlayer->not_on_water)
    {
        turnHeldTime = 0;

        if (pPlayer->tilt_status > 0)
            pPlayer->tilt_status--;
        else if (pPlayer->tilt_status < 0)
            pPlayer->tilt_status++;
    }

    input.q16avel      = fix16_mul(input.q16avel, avelScale);
    localInput.q16avel = fix16_sadd(localInput.q16avel, input.q16avel);
    pPlayer->q16ang    = fix16_sadd(pPlayer->q16ang, input.q16avel) & 0x7FFFFFF;
    localInput.fvel    = clamp((input.fvel += pPlayer->MotoSpeed), -(MAXVELMOTO / 8), MAXVELMOTO);
}

int dword_A99D4, dword_A99D8, dword_A99DC, dword_A99E0;
int dword_164620, dword_164624;

void sub_299C0(void)
{
    dword_A99D8 = 0;
    dword_A99DC = 0;
}

int sub_299D8(void)
{
    if ((int)totalclock - dword_A99D8 >= 30 && buttonMap.ButtonDown(gamefunc_Crouch))
    {
        dword_A99D8 = (int)totalclock;
        dword_A99DC ^= 1;
    }
    return dword_A99DC;
}

void madenoise(int playerNum)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;
    pPlayer->make_noise = 1;
    pPlayer->noise_x = pPlayer->pos.x;
    pPlayer->noise_y = pPlayer->pos.y;
}

static void P_Thrust(DukePlayer_t *const pPlayer, int shift)
{
    pPlayer->vel.x += sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047] << shift;
    pPlayer->vel.y += sintable[fix16_to_int(pPlayer->q16ang)&2047] << shift;
}

int16_t WeaponPickupSprites[MAX_WEAPONS] = { KNEE__STATIC, FIRSTGUNSPRITE__STATIC, SHOTGUNSPRITE__STATIC,
        CHAINGUNSPRITE__STATIC, RPGSPRITE__STATIC, HEAVYHBOMB__STATIC, SHRINKERSPRITE__STATIC, DEVISTATORSPRITE__STATIC,
        TRIPBOMBSPRITE__STATIC, FREEZESPRITE__STATIC, HEAVYHBOMB__STATIC, SHRINKERSPRITE__STATIC
                                           };
// this is used for player deaths
void checkweapons(DukePlayer_t* const pPlayer)
{
    int playerNum = sprite[pPlayer->i].yvel;
    int const currentWeapon = WW2GI ? PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) : pPlayer->curr_weapon;

    if (RRRA && (g_netServer || numplayers > 1))
    {
        if (pPlayer->OnMotorcycle)
        {
            int const newSprite = fi.spawn(pPlayer->i, TILE_EMPTYBIKE);
            sprite[newSprite].ang = fix16_to_int(pPlayer->q16ang);
            sprite[newSprite].owner = pPlayer->ammo_amount[MOTORCYCLE_WEAPON];
            pPlayer->OnMotorcycle = 0;
            pPlayer->gotweapon.Clear(MOTORCYCLE_WEAPON);
            pPlayer->q16horiz = F16(100);
            pPlayer->moto_do_bump = 0;
            pPlayer->MotoSpeed = 0;
            pPlayer->tilt_status = 0;
            pPlayer->moto_drink = 0;
            pPlayer->moto_bump_target = 0;
            pPlayer->moto_bump = 0;
            pPlayer->moto_turb = 0;
        }
        else if (pPlayer->OnBoat)
        {
            int const newSprite = fi.spawn(pPlayer->i, TILE_EMPTYBOAT);
            sprite[newSprite].ang = fix16_to_int(pPlayer->q16ang);
            sprite[newSprite].owner = pPlayer->ammo_amount[BOAT_WEAPON];
            pPlayer->OnBoat = 0;
            pPlayer->gotweapon.Clear(BOAT_WEAPON);
            pPlayer->q16horiz = F16(100);
            pPlayer->moto_do_bump = 0;
            pPlayer->MotoSpeed = 0;
            pPlayer->tilt_status = 0;
            pPlayer->moto_drink = 0;
            pPlayer->moto_bump_target = 0;
            pPlayer->moto_bump = 0;
            pPlayer->moto_turb = 0;
        }
    }

    if (currentWeapon == KNEE_WEAPON || (unsigned)currentWeapon >= MAX_WEAPONS)
        return;

    if (krand2() & 1)
        fi.spawn(pPlayer->i, WeaponPickupSprites[currentWeapon]);
    else
        switch (DYNAMICWEAPONMAP(currentWeapon))
        {
            case CHICKEN_WEAPON__STATIC:
                if (!RRRA) break;
                fallthrough__;
            case RPG_WEAPON__STATIC:
            case HANDBOMB_WEAPON__STATIC: fi.spawn(pPlayer->i, TILE_EXPLOSION2); break;
        }

    if (RR)
    {
        for (bssize_t key = 0; key < 5; key++)
        {
            if (pPlayer->keys[key] == 1)
            {
                int const newSprite = fi.spawn(pPlayer->i, TILE_ACCESSCARD);
                switch (key)
                {
                    case 1:
                        sprite[newSprite].lotag = 100;
                        break;
                    case 2:
                        sprite[newSprite].lotag = 101;
                        break;
                    case 3:
                        sprite[newSprite].lotag = 102;
                        break;
                    case 4:
                        sprite[newSprite].lotag = 103;
                        break;
                }
            }
        }
    }
}

void P_AddAmmo(DukePlayer_t * const pPlayer, int const weaponNum, int const addAmount)
{
    pPlayer->ammo_amount[weaponNum] += addAmount;

    if (pPlayer->ammo_amount[weaponNum] > max_ammo_amount[weaponNum])
        pPlayer->ammo_amount[weaponNum] = max_ammo_amount[weaponNum];
}

void checkavailweapon(struct player_struct* p);

void P_AddWeapon(DukePlayer_t *pPlayer, int weaponNum)
{
    fi.addweapon(pPlayer, weaponNum);
}

void P_SelectNextInvItem(DukePlayer_t *pPlayer)
{
    checkavailinven(pPlayer);
}

void P_CheckWeapon(DukePlayer_t *pPlayer)
{
    checkavailweapon(pPlayer);
}

static void DoWallTouchDamage(const DukePlayer_t *pPlayer, int32_t wallNum)
{
    vec3_t const davect = { pPlayer->pos.x + (sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047] >> 9),
                      pPlayer->pos.y + (sintable[fix16_to_int(pPlayer->q16ang) & 2047] >> 9), pPlayer->pos.z };

    fi.checkhitwall(pPlayer->i, wallNum, davect.x, davect.y, davect.z, -1);
}

static void P_CheckTouchDamage(DukePlayer_t *pPlayer, int touchObject)
{
    if (touchObject == -1)
        return;

    if ((touchObject & 49152) == 49152)
    {
        int const touchSprite = touchObject & (MAXSPRITES - 1);

        if (RRRA)
        {
            switch (DYNAMICTILEMAP(sprite[touchSprite].picnum))
            {
                case RRTILE2430__STATICRR:
                case RRTILE2431__STATICRR:
                case RRTILE2432__STATICRR:
                case RRTILE2443__STATICRR:
                case RRTILE2446__STATICRR:
                case RRTILE2451__STATICRR:
                case RRTILE2455__STATICRR:
                    if (pPlayer->hurt_delay2 < 8)
                    {
                        sprite[pPlayer->i].extra -= 5;

                        pPlayer->hurt_delay2 = 16;
                        P_PalFrom(pPlayer, 32, 32, 0, 0);
                        A_PlaySound(DUKE_LONGTERM_PAIN, pPlayer->i);
                    }
                    break;
            }
            return;
        }
        if (sprite[touchSprite].picnum == TILE_CACTUS)
        {
            if (pPlayer->hurt_delay < 8)
            {
                sprite[pPlayer->i].extra -= 5;

                pPlayer->hurt_delay = 16;
                P_PalFrom(pPlayer, 32, 32, 0, 0);
                A_PlaySound(DUKE_LONGTERM_PAIN, pPlayer->i);
            }
        }
        return;
    }

    if ((touchObject & 49152) != 32768)
        return;

    int const touchWall = touchObject & (MAXWALLS-1);

    if (pPlayer->hurt_delay > 0)
        pPlayer->hurt_delay--;
    else if (wall[touchWall].cstat & FORCEFIELD_CSTAT)
    {
        int const forcePic = G_GetForcefieldPicnum(touchWall);

        switch (DYNAMICTILEMAP(forcePic))
        {
        case W_FORCEFIELD__STATIC:
            if (RR) break;
            sprite[pPlayer->i].extra -= 5;

            pPlayer->hurt_delay = 16;
            P_PalFrom(pPlayer, 32, 32,0,0);

            pPlayer->vel.x = -(sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047]<<8);
            pPlayer->vel.y = -(sintable[(fix16_to_int(pPlayer->q16ang))&2047]<<8);
            A_PlaySound(DUKE_LONGTERM_PAIN,pPlayer->i);

            DoWallTouchDamage(pPlayer, touchWall);
            break;

        case BIGFORCE__STATIC:
            pPlayer->hurt_delay = GAMETICSPERSEC;
            DoWallTouchDamage(pPlayer, touchWall);
            break;
        }
    }
}

static int P_CheckFloorDamage(DukePlayer_t *pPlayer, int floorTexture)
{
    spritetype * const pSprite = &sprite[pPlayer->i];

    if ((unsigned)(floorTexture) >= MAXTILES)
        return 0;

    switch (DYNAMICTILEMAP(floorTexture))
    {
        case HURTRAIL__STATIC:
            if (rnd(32))
            {
                if (pPlayer->inv_amount[GET_BOOTS] > 0)
                    return 1;
                else
                {
                    if (!A_CheckSoundPlaying(pPlayer->i, DUKE_LONGTERM_PAIN))
                        A_PlaySound(DUKE_LONGTERM_PAIN, pPlayer->i);

                    P_PalFrom(pPlayer, 32, 64, 64, 64);

                    pSprite->extra -= 1 + (krand2() & 3);
                    if (!A_CheckSoundPlaying(pPlayer->i, SHORT_CIRCUIT))
                        A_PlaySound(SHORT_CIRCUIT, pPlayer->i);

                    return 0;
                }
            }
            break;

        case FLOORSLIME__STATIC:
            if (rnd(16))
            {
                if (pPlayer->inv_amount[GET_BOOTS] > 0)
                    return 1;
                else
                {
                    if (!A_CheckSoundPlaying(pPlayer->i, DUKE_LONGTERM_PAIN))
                        A_PlaySound(DUKE_LONGTERM_PAIN, pPlayer->i);

                    P_PalFrom(pPlayer, 32, 0, 8, 0);
                    pSprite->extra -= 1 + (krand2() & 3);

                    return 0;
                }
            }
            break;

        case FLOORPLASMA__STATIC:
            if (rnd(32))
            {
                if (pPlayer->inv_amount[GET_BOOTS] > 0)
                    return 1;
                else
                {
                    if (!A_CheckSoundPlaying(pPlayer->i, DUKE_LONGTERM_PAIN))
                        A_PlaySound(DUKE_LONGTERM_PAIN, pPlayer->i);

                    P_PalFrom(pPlayer, 32, 8, 0, 0);
                    pSprite->extra -= 1 + (krand2() & 3);

                    return 0;
                }
            }
            break;
    }

    return 0;
}



void P_FragPlayer(int playerNum)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;
    spritetype *const   pSprite = &sprite[pPlayer->i];

    //if (g_netClient) // [75] The server should not overwrite its own randomseed
    //    randomseed = ticrandomseed;

    if (pSprite->pal != 1)
    {
        P_PalFrom(pPlayer, 63, 63, 0, 0);

        pPlayer->pos.z -= ZOFFSET2;
        pSprite->z -= ZOFFSET2;

        int32_t const r1 = krand2(), r2 = krand2();
        pPlayer->dead_flag = (512 - ((r2 & 1) << 10) + (r1 & 255) - 512) & 2047;

        if (pPlayer->dead_flag == 0)
            pPlayer->dead_flag++;

#ifndef NETCODE_DISABLE
        //if (g_netServer)
        //{
        //    packbuf[0] = PACKET_FRAG;
        //    packbuf[1] = playerNum;
        //    packbuf[2] = pPlayer->frag_ps;
        //    packbuf[3] = actor[pPlayer->i].picnum;
        //    B_BUF32(&packbuf[4], ticrandomseed);
        //    packbuf[8] = myconnectindex;
        //
        //    enet_host_broadcast(g_netServer, CHAN_GAMESTATE, enet_packet_create(packbuf, 9, ENET_PACKET_FLAG_RELIABLE));
        //}
#endif
    }

    pPlayer->jetpack_on  = 0;
    pPlayer->holoduke_on = -1;

    if (!RR)
        S_StopEnvSound(DUKE_JETPACK_IDLE, pPlayer->i);

    S_StopEnvSound(-1, pPlayer->i, CHAN_VOICE);

    if (pSprite->pal != 1 && (pSprite->cstat & 32768) == 0)
        pSprite->cstat = 0;

    if ((g_netServer || ud.multimode > 1) && (pSprite->pal != 1 || (pSprite->cstat & 32768)))
    {
        if (pPlayer->frag_ps != playerNum)
        {
            if (GTFLAGS(GAMETYPE_TDM) && g_player[pPlayer->frag_ps].ps->team == g_player[playerNum].ps->team)
                g_player[pPlayer->frag_ps].ps->fraggedself++;
            else
            {
                g_player[pPlayer->frag_ps].ps->frag++;
                g_player[pPlayer->frag_ps].frags[playerNum]++;
                g_player[playerNum].frags[playerNum]++;  // deaths
            }

            if (playerNum == screenpeek)
            {
				quoteMgr.InitializeQuote(QUOTE_RESERVED, "Killed by %s", &g_player[pPlayer->frag_ps].user_name[0]);
                P_DoQuote(QUOTE_RESERVED, pPlayer);
            }
            else
            {
				quoteMgr.InitializeQuote(QUOTE_RESERVED2, "Killed %s", &g_player[playerNum].user_name[0]);
                P_DoQuote(QUOTE_RESERVED2, g_player[pPlayer->frag_ps].ps);
            }
        }
        pPlayer->frag_ps = playerNum;
        pus              = NUMPAGES;
    }
}

# define PIPEBOMB_CONTROL(playerNum) (Gv_GetVarByLabel("PIPEBOMB_CONTROL", PIPEBOMB_REMOTE, -1, playerNum))

static void P_ProcessWeapon(int playerNum)
{
    DukePlayer_t *const pPlayer      = g_player[playerNum].ps;
    uint8_t *const      weaponFrame  = &pPlayer->kickback_pic;
    int const           playerShrunk = (sprite[pPlayer->i].yrepeat < (RR ? 8 : 32));
    uint32_t            playerBits   = g_player[playerNum].input->bits;
    int const           sectorLotag  = sector[pPlayer->cursectnum].lotag;

    if (RR)
    {
        if (pPlayer->detonate_count > 0)
        {
            if (ud.god)
            {
                pPlayer->detonate_time = 45;
                pPlayer->detonate_count = 0;
            }
            else if (pPlayer->detonate_time <= 0 && (*weaponFrame) < 5)
            {
                S_PlaySound(PIPEBOMB_EXPLODE);
                quickkill(pPlayer);
            }
        }
    }
#define WEAPON2_CLIP 20
    if (NAM_WW2GI && TEST_SYNC_KEY(playerBits, SK_HOLSTER))   // 'Holster Weapon
    {
        if (NAM)
        {
            if (pPlayer->curr_weapon == PISTOL_WEAPON)
            {
                if (pPlayer->ammo_amount[PISTOL_WEAPON] > WEAPON2_CLIP)
                {
                    // throw away the remaining clip
                    pPlayer->ammo_amount[PISTOL_WEAPON] -= pPlayer->ammo_amount[PISTOL_WEAPON] % WEAPON2_CLIP;
                    (*weaponFrame) = 3;
                    playerBits &= ~BIT(SK_FIRE);
                }
                return;
            }
        }
        else
        {
            P_SetWeaponGamevars(playerNum, pPlayer);

            if (VM_OnEvent(EVENT_HOLSTER, pPlayer->i, playerNum) == 0)
            {
                if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_HOLSTER_CLEARS_CLIP)
                {
                    int const weap = pPlayer->curr_weapon, clipcnt = PWEAPON(playerNum, weap, Clip);

                    if (pPlayer->ammo_amount[weap] > clipcnt && (pPlayer->ammo_amount[weap] % clipcnt) != 0)
                    {
                        pPlayer->ammo_amount[weap] -= pPlayer->ammo_amount[weap] % clipcnt;
                        *weaponFrame                = PWEAPON(playerNum, weap, TotalTime)+1;
                        playerBits                 &= ~BIT(SK_FIRE);  // not firing...
                    }

                    return;
                }
            }
        }
    }
#undef WEAPON2_CLIP
    if (WW2GI ? PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_GLOWS :
        (pPlayer->curr_weapon == SHRINKER_WEAPON || pPlayer->curr_weapon == GROW_WEAPON
        || (RR && (pPlayer->curr_weapon == TRIPBOMB_WEAPON || pPlayer->curr_weapon == BOWLING_WEAPON))
        || (RRRA && (pPlayer->curr_weapon == KNEE_WEAPON || pPlayer->curr_weapon == SLINGBLADE_WEAPON))))
    {
        pPlayer->random_club_frame += 64; // Glowing

#ifdef POLYMER
        if (pPlayer->kickback_pic == 0 && !RR)
        {
            spritetype *const pSprite     = &sprite[pPlayer->i];
            int const         glowXOffset = ((sintable[(pSprite->ang + 512) & 2047]) >> 7);
            int const         glowYOffset = ((sintable[(pSprite->ang) & 2047]) >> 7);
            int const         glowRange   = 1024 + (sintable[pPlayer->random_club_frame & 2047] >> 3);
            int const         flashColor  = (pPlayer->curr_weapon == GROW_WEAPON) ? 216+(52<<8)+(20<<16) : 176+(252<<8)+(120<<16);

            pSprite->x += glowXOffset;
            pSprite->y += glowYOffset;

            G_AddGameLight(0, pPlayer->i, PHEIGHT, max(glowRange, 0), flashColor, PR_LIGHT_PRIO_HIGH_GAME);

            actor[pPlayer->i].lightcount = 2;

            pSprite->x -= glowXOffset;
            pSprite->y -= glowYOffset;
        }
#endif
    }

    if (pPlayer->rapid_fire_hold == 1)
    {
        if (TEST_SYNC_KEY(playerBits, SK_FIRE))
            return;
        pPlayer->rapid_fire_hold = 0;
    }

    if (playerShrunk || pPlayer->tipincs || pPlayer->access_incs)
        playerBits &= ~BIT(SK_FIRE);
    else if (playerShrunk == 0 && (playerBits & (1 << 2)) && (*weaponFrame) == 0 && pPlayer->fist_incs == 0 &&
             pPlayer->last_weapon == -1 && (pPlayer->weapon_pos == 0 || pPlayer->holster_weapon == 1))
    {
        pPlayer->crack_time = 777;

        if (pPlayer->holster_weapon == 1)
        {
            if (pPlayer->last_pissed_time <= (GAMETICSPERSEC * 218) && pPlayer->weapon_pos == WEAPON_POS_LOWER)
            {
                pPlayer->holster_weapon = 0;
                pPlayer->weapon_pos     = WEAPON_POS_RAISE;
                P_DoQuote(QUOTE_WEAPON_RAISED, pPlayer);
            }
        }
        else
        {
            P_SetWeaponGamevars(playerNum, pPlayer);
            if (VM_OnEvent(EVENT_FIRE, pPlayer->i, playerNum) == 0)
            {
                switch (DYNAMICWEAPONMAP(WW2GI ? PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) : pPlayer->curr_weapon))
                {
                    case HANDBOMB_WEAPON__STATIC:
                        pPlayer->hbomb_hold_delay = 0;
                        if (pPlayer->ammo_amount[pPlayer->curr_weapon] > 0)
                        {
                            (*weaponFrame) = 1;
                            if (WW2GI && PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                                A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                        }
                        break;

                    case HANDREMOTE_WEAPON__STATIC:
                        pPlayer->hbomb_hold_delay = 0;
                        (*weaponFrame)            = 1;
                        if (WW2GI && PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                            A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                        break;

                    case PISTOL_WEAPON__STATIC:
                        if (pPlayer->ammo_amount[PISTOL_WEAPON] > 0)
                        {
                            if (!WW2GI)
                                pPlayer->ammo_amount[PISTOL_WEAPON]--;
                            (*weaponFrame) = 1;
                            if (WW2GI && PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                                A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                        }
                        break;

                    case SHOTGUN_WEAPON__STATIC:
                        if (pPlayer->ammo_amount[SHOTGUN_WEAPON] > 0 && pPlayer->random_club_frame == 0)
                        {
                            (*weaponFrame) = 1;
                            if (WW2GI && PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                                A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                        }
                        break;

                    case TRIPBOMB_WEAPON__STATIC:
                    case BOWLINGBALL_WEAPON__STATIC:
                        if (RR)
                        {
                            if (pPlayer->ammo_amount[pPlayer->curr_weapon] > 0)
                            {
                                //pPlayer->ammo_amount[pPlayer->curr_weapon]--;
                                (*weaponFrame) = 1;
                            }
                            break;
                        }
                        if (pPlayer->curr_weapon == TILE_BOWLINGBALL)
                            break;
                        if (pPlayer->ammo_amount[pPlayer->curr_weapon] > 0)
                        {
                            hitdata_t hitData;

                            hitscan((const vec3_t *)pPlayer, pPlayer->cursectnum, sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047],
                                    sintable[fix16_to_int(pPlayer->q16ang) & 2047], fix16_to_int(F16(100) - pPlayer->q16horiz - pPlayer->q16horizoff) * 32, &hitData,
                                    CLIPMASK1);

                            if ((hitData.sect < 0 || hitData.sprite >= 0) ||
                                (hitData.wall >= 0 && sector[hitData.sect].lotag > 2))
                                break;

                            if (hitData.wall >= 0 && wall[hitData.wall].overpicnum >= 0)
                                if (wall[hitData.wall].overpicnum == TILE_BIGFORCE)
                                    break;

                            int spriteNum = headspritesect[hitData.sect];
                            while (spriteNum >= 0)
                            {
                                if (sprite[spriteNum].picnum == TILE_TRIPBOMB && klabs(sprite[spriteNum].z - hitData.pos.z) < ZOFFSET4 &&
                                    ((sprite[spriteNum].x - hitData.pos.x) * (sprite[spriteNum].x - hitData.pos.x) +
                                        (sprite[spriteNum].y - hitData.pos.y) * (sprite[spriteNum].y - hitData.pos.y)) < (290 * 290))
                                    break;
                                spriteNum = nextspritesect[spriteNum];
                            }

                            // ST_2_UNDERWATER
                            if (spriteNum == -1 && hitData.wall >= 0 && (wall[hitData.wall].cstat & 16) == 0)
                                if ((wall[hitData.wall].nextsector >= 0 && sector[wall[hitData.wall].nextsector].lotag <= 2) ||
                                    (wall[hitData.wall].nextsector == -1 && sector[hitData.sect].lotag <= 2))
                                    if (((hitData.pos.x - pPlayer->pos.x) * (hitData.pos.x - pPlayer->pos.x) +
                                            (hitData.pos.y - pPlayer->pos.y) * (hitData.pos.y - pPlayer->pos.y)) < (290 * 290))
                                    {
                                        pPlayer->pos.z = pPlayer->opos.z;
                                        pPlayer->vel.z = 0;
                                        (*weaponFrame) = 1;
                                        if (WW2GI && PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                                        {
                                            A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                                        }
                                    }
                        }
                        break;

                    case SHRINKER_WEAPON__STATIC:
                        if (pPlayer->ammo_amount[SHRINKER_WEAPON] > 0)
                        {
                            (*weaponFrame) = 1;
                            if (!WW2GI)
                                A_PlaySound(SHRINKER_FIRE, pPlayer->i);
                            else if (PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                                A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                        }
                        break;

                    case GROW_WEAPON__STATIC:
                        if (pPlayer->ammo_amount[GROW_WEAPON] > 0)
                        {
                            (*weaponFrame) = 1;
                            if (!WW2GI)
                                A_PlaySound(RR ? 431 : EXPANDERSHOOT, pPlayer->i);
                            else if (PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                                A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                        }
                        break;

                    case FREEZE_WEAPON__STATIC:
                        if (pPlayer->ammo_amount[pPlayer->curr_weapon] > 0)
                        {
                            (*weaponFrame) = 1;
                            if (!RR)
                            {
                                if (!WW2GI)
                                    A_PlaySound(CAT_FIRE, pPlayer->i);
                                else if (PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                                    A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                            }
                        }
                        break;

                    case RPG_WEAPON__STATIC:
                    case CHAINGUN_WEAPON__STATIC:
                        if (pPlayer->ammo_amount[pPlayer->curr_weapon] > 0)
                        {
                            (*weaponFrame) = 1;
                            if (WW2GI && PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                                A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                        }
                        break;

                    case DEVISTATOR_WEAPON__STATIC:
                        if (pPlayer->ammo_amount[pPlayer->curr_weapon] > 0)
                        {
                            (*weaponFrame)            = 1;
                            pPlayer->hbomb_hold_delay = !pPlayer->hbomb_hold_delay;
                            if (!RR)
                            {
                                if (!WW2GI)
                                    A_PlaySound(CAT_FIRE, pPlayer->i);
                                else if (PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                                    A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                            }
                        }
                        break;

                    case KNEE_WEAPON__STATIC:
                    case SLINGBLADE_WEAPON__STATIC:
                        if (RRRA)
                        {
                            if (pPlayer->ammo_amount[pPlayer->curr_weapon] > 0 && !pPlayer->quick_kick)
                                (*weaponFrame) = 1;
                            break;
                        }
                        if (RRRA && pPlayer->curr_weapon == TILE_SLINGBLADE) break;
                        if (pPlayer->quick_kick == 0)
                        {
                            (*weaponFrame) = 1;
                            if (WW2GI && PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound) > 0)
                                A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, InitialSound), pPlayer->i);
                        }
                        break;

                    case CHICKEN_WEAPON__STATIC:
                    case MOTORCYCLE_WEAPON__STATIC:
                    case BOAT_WEAPON__STATIC:
                        if (!RRRA) break;
                        if (pPlayer->ammo_amount[pPlayer->curr_weapon] > 0)
                        {
                            (*weaponFrame) = 1;
                        }
                        break;
                }
            }
        }
    }
    else if (*weaponFrame)
    {
        int spriteNum;
        int flashColor = 0;

        if (RR)
        {
            switch (DYNAMICWEAPONMAP(pPlayer->curr_weapon))
            {
            case HANDBOMB_WEAPON__STATIC:
                if (*weaponFrame == 1)
                    S_PlaySound(401);

                if ((*weaponFrame) == 6 && TEST_SYNC_KEY(playerBits, SK_FIRE))
                    pPlayer->rapid_fire_hold = 1;

                if (++(*weaponFrame) > 19)
                {
                    *weaponFrame = 0;
                    pPlayer->curr_weapon = DYNAMITE_WEAPON;
                    pPlayer->last_weapon = -1;
                    pPlayer->weapon_pos = WEAPON_POS_RAISE;
                    pPlayer->detonate_time = 45;
                    pPlayer->detonate_count = 1;
                    S_PlaySound(402);
                }
                break;

            case HANDREMOTE_WEAPON__STATIC:
                (*weaponFrame)++;

                if (pPlayer->detonate_time < 0)
                    pPlayer->hbomb_on = 0;

                if ((*weaponFrame) == 39)
                {
                    pPlayer->hbomb_on = 0;
                    pPlayer->noise_radius = 8192;
                    madenoise(playerNum);
                }
                if ((*weaponFrame) == 12)
                {
                    pPlayer->ammo_amount[DYNAMITE_WEAPON]--;
                    if (pPlayer->ammo_amount[CROSSBOW_WEAPON])
                        pPlayer->ammo_amount[CROSSBOW_WEAPON]--;

                    //if (numplayers < 2 || g_netServer)
                    {
                        int pipeBombZvel;
                        int pipeBombFwdVel;

                        if (pPlayer->on_ground && TEST_SYNC_KEY(playerBits, SK_CROUCH))
                        {
                            pipeBombFwdVel = 15;
                            pipeBombZvel   = (fix16_to_int(pPlayer->q16horiz + pPlayer->q16horizoff - F16(100)) * 20);
                        }
                        else
                        {
                            pipeBombFwdVel = 140;
                            pipeBombZvel   = -512 - (fix16_to_int(pPlayer->q16horiz + pPlayer->q16horizoff - F16(100)) * 20);
                        }

                        int pipeSpriteNum = A_InsertSprite(pPlayer->cursectnum,
                                           pPlayer->pos.x+(sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047]>>6),
                                           pPlayer->pos.y+(sintable[fix16_to_int(pPlayer->q16ang)&2047]>>6),
                                           pPlayer->pos.z,TILE_HEAVYHBOMB,-16,9,9,
                                           fix16_to_int(pPlayer->q16ang),(pipeBombFwdVel+(pPlayer->hbomb_hold_delay<<5))*2,pipeBombZvel,pPlayer->i,1);

                        if (pipeBombFwdVel == 15)
                        {
                            sprite[pipeSpriteNum].yvel = 3;
                            sprite[pipeSpriteNum].z += ZOFFSET3;
                        }

                        if (hits(pPlayer->i) < 512)
                        {
                            sprite[pipeSpriteNum].ang += 1024;
                            sprite[pipeSpriteNum].zvel /= 3;
                            sprite[pipeSpriteNum].xvel /= 3;
                        }
                    }

                    pPlayer->hbomb_on = 1;
                }
                else if (*weaponFrame < 12 && TEST_SYNC_KEY(playerBits, SK_FIRE))
                    pPlayer->hbomb_hold_delay++;

                if (*weaponFrame == 40)
                {
                    (*weaponFrame) = 0;
                    pPlayer->curr_weapon = HANDBOMB_WEAPON;
                    pPlayer->last_weapon = -1;
                    pPlayer->detonate_count = 0;
                    pPlayer->detonate_time = 45;
                    if (pPlayer->ammo_amount[HANDBOMB_WEAPON] > 0)
                    {
                        P_AddWeapon(pPlayer, HANDBOMB_WEAPON);
                        pPlayer->weapon_pos = WEAPON_POS_LOWER;
                    }
                    else
                        P_CheckWeapon(pPlayer);
                }
                break;

            case PISTOL_WEAPON__STATIC:
                if ((*weaponFrame) == 1)
                {
                    fi.shoot(pPlayer->i, TILE_SHOTSPARK1);
                    A_PlaySound(PISTOL_FIRE, pPlayer->i);
                    pPlayer->noise_radius = 8192;
                    madenoise(playerNum);

                    lastvisinc = (int32_t) totalclock+32;
                    pPlayer->visibility = 0;
                    flashColor = 255+(95<<8);
                    if (sectorLotag != 857)
                    {
                        pPlayer->vel.x -= sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047] << 4;
                        pPlayer->vel.y -= sintable[fix16_to_int(pPlayer->q16ang) & 2047] << 4;
                    }
                }
                else if ((*weaponFrame) == 2 && pPlayer->ammo_amount[PISTOL_WEAPON] <= 0)
                {
                    (*weaponFrame) = 0;
                    P_CheckWeapon(pPlayer);
                }

                if (++(*weaponFrame) >= 22)
                {
                    if (pPlayer->ammo_amount[PISTOL_WEAPON] <= 0)
                    {
                        (*weaponFrame) = 0;
                        P_CheckWeapon(pPlayer);
                        break;
                    }
                    else if ((pPlayer->ammo_amount[PISTOL_WEAPON]%6) == 0)
                    {
                        switch ((*weaponFrame))
                        {
                        case 24:
                            A_PlaySound(EJECT_CLIP, pPlayer->i);
                            break;
                        case 30:
                            A_PlaySound(INSERT_CLIP, pPlayer->i);
                            break;
                        }
                    }
                    else
                        (*weaponFrame) = 38;
                }

                if ((*weaponFrame) == 38)
                {
                    (*weaponFrame) = 0;
                    P_CheckWeapon(pPlayer);
                }
                break;

            case SHOTGUN_WEAPON__STATIC:

                if (++(*weaponFrame) == 6 && pPlayer->shotgun_state[0] == 0 && pPlayer->ammo_amount[SHOTGUN_WEAPON] > 1 && TEST_SYNC_KEY(playerBits, SK_FIRE))
                    pPlayer->shotgun_state[1] = 1;
                if ((*weaponFrame) == 4)
                {
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);

                    pPlayer->ammo_amount[SHOTGUN_WEAPON]--;

                    A_PlaySound(SHOTGUN_FIRE, pPlayer->i);

                    pPlayer->noise_radius = 8192;
                    madenoise(playerNum);

                    lastvisinc = (int32_t) totalclock + 32;
                    pPlayer->visibility = 0;
                    flashColor = 255+(95<<8);
                }

                if ((*weaponFrame) == 7)
                {
                    if (pPlayer->shotgun_state[1])
                    {
                        fi.shoot(pPlayer->i, TILE_SHOTGUN);
                        fi.shoot(pPlayer->i, TILE_SHOTGUN);
                        fi.shoot(pPlayer->i, TILE_SHOTGUN);
                        fi.shoot(pPlayer->i, TILE_SHOTGUN);
                        fi.shoot(pPlayer->i, TILE_SHOTGUN);
                        fi.shoot(pPlayer->i, TILE_SHOTGUN);
                        fi.shoot(pPlayer->i, TILE_SHOTGUN);
                        fi.shoot(pPlayer->i, TILE_SHOTGUN);
                        fi.shoot(pPlayer->i, TILE_SHOTGUN);
                        fi.shoot(pPlayer->i, TILE_SHOTGUN);

                        pPlayer->ammo_amount[SHOTGUN_WEAPON]--;

                        A_PlaySound(SHOTGUN_FIRE, pPlayer->i);
                        if (sectorLotag != 857)
                        {
                            pPlayer->vel.x -= sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047] << 5;
                            pPlayer->vel.y -= sintable[fix16_to_int(pPlayer->q16ang) & 2047] << 5;
                        }
                    } else if (sectorLotag != 857)
                    {
                        pPlayer->vel.x -= sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047] << 4;
                        pPlayer->vel.y -= sintable[fix16_to_int(pPlayer->q16ang) & 2047] << 4;
                    }
                    flashColor = 255+(95<<8);
                }

                if (pPlayer->shotgun_state[0])
                {
                    switch (*weaponFrame)
                    {
                    case 16:
                        P_CheckWeapon(pPlayer);
                        break;
                    case 17:
                        A_PlaySound(SHOTGUN_COCK, pPlayer->i);
                        break;
                    case 28:
                        *weaponFrame = 0;
                        pPlayer->shotgun_state[0] = 0;
                        pPlayer->shotgun_state[1] = 0;
                        break;
                    }
                }
                else if (pPlayer->shotgun_state[1])
                {
                    switch (*weaponFrame)
                    {
                    case 26:
                        P_CheckWeapon(pPlayer);
                        break;
                    case 27:
                        A_PlaySound(SHOTGUN_COCK, pPlayer->i);
                        break;
                    case 38:
                        *weaponFrame = 0;
                        pPlayer->shotgun_state[0] = 0;
                        pPlayer->shotgun_state[1] = 0;
                        break;
                    }
                }
                else
                {
                    switch (*weaponFrame)
                    {
                    case 16:
                        P_CheckWeapon(pPlayer);
                        *weaponFrame = 0;
                        pPlayer->shotgun_state[0] = 1;
                        pPlayer->shotgun_state[1] = 0;
                        break;
                    }
                }
                break;

            case CHAINGUN_WEAPON__STATIC:
                pPlayer->q16horiz += F16(1);
                pPlayer->recoil++;
                if (++(*weaponFrame) <= 12)
                {
                    if (((*weaponFrame) % 3) == 0)
                    {
                        pPlayer->ammo_amount[CHAINGUN_WEAPON]--;

                        if (((*weaponFrame) % 3) == 0)
                        {
                            spriteNum = fi.spawn(pPlayer->i, TILE_SHELL);

                            sprite[spriteNum].ang += 1024;
                            sprite[spriteNum].ang &= 2047;
                            sprite[spriteNum].xvel += 32;
                            sprite[spriteNum].z += (3 << 8);
                            A_SetSprite(spriteNum, CLIPMASK0);
                        }

                        A_PlaySound(CHAINGUN_FIRE, pPlayer->i);
                        fi.shoot(pPlayer->i, TILE_CHAINGUN);
                        pPlayer->noise_radius = 8192;
                        madenoise(playerNum);
                        lastvisinc = (int32_t) totalclock + 32;
                        pPlayer->visibility = 0;
                        flashColor = 255+(95<<8);
                        if (sectorLotag != 857)
                        {
                            pPlayer->vel.x -= sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047] << 4;
                            pPlayer->vel.y -= sintable[fix16_to_int(pPlayer->q16ang) & 2047] << 4;
                        }
                        P_CheckWeapon(pPlayer);

                        if (!TEST_SYNC_KEY(playerBits, SK_FIRE))
                        {
                            (*weaponFrame) = 0;
                            break;
                        }
                    }
                }
                else if ((*weaponFrame) > 10)
                {
                    if (TEST_SYNC_KEY(playerBits, SK_FIRE))
                    {
                        (*weaponFrame) = 1;
                    }
                    else
                    {
                        (*weaponFrame) = 0;
                    }
                }

                break;

            case GROW_WEAPON__STATIC:
                if ((*weaponFrame) > 3)
                {
                    (*weaponFrame) = 0;
                    if (screenpeek == playerNum)
                    {
                        pus = 1;
                    }

                    fi.shoot(pPlayer->i, TILE_GROWSPARK);

                    pPlayer->noise_radius = 1024;
                    madenoise(playerNum);
                    P_CheckWeapon(pPlayer);
                }
                else
                {
                    (*weaponFrame)++;
                }
                break;

            case SHRINKER_WEAPON__STATIC:
                if ((*weaponFrame) == 1)
                {
                    pPlayer->ammo_amount[SHRINKER_WEAPON]--;

                    fi.shoot(pPlayer->i, TILE_SHRINKSPARK);
                    P_CheckWeapon(pPlayer);
                }
                if (++(*weaponFrame) > 20)
                    (*weaponFrame) = 0;
                break;

            case DEVISTATOR_WEAPON__STATIC:
                (*weaponFrame)++;
                if ((*weaponFrame) == 2 || (*weaponFrame) == 4)
                {
                    pPlayer->visibility = 0;
                    flashColor = 255 + (95 << 8);
                    lastvisinc = (int32_t) totalclock + 32;
                    A_PlaySound(CHAINGUN_FIRE, pPlayer->i);
                    fi.shoot(pPlayer->i, TILE_SHOTSPARK1);
                    pPlayer->noise_radius = 16384;
                    madenoise(playerNum);
                    pPlayer->ammo_amount[DEVISTATOR_WEAPON]--;
                    P_CheckWeapon(pPlayer);
                }
                if ((*weaponFrame) == 2)
                {
                    pPlayer->q16ang += F16(16);
                }
                else if ((*weaponFrame) == 4)
                {
                    pPlayer->q16ang -= F16(16);
                }
                if ((*weaponFrame) > 4)
                    (*weaponFrame) = 1;
                if (!TEST_SYNC_KEY(playerBits, SK_FIRE))
                    (*weaponFrame) = 0;
                break;

            case MOTORCYCLE_WEAPON__STATIC:
                if (!RRRA) break;
                (*weaponFrame)++;
                if ((*weaponFrame) == 2 || (*weaponFrame) == 4)
                {
                    pPlayer->visibility = 0;
                    flashColor = 255 + (95 << 8);
                    lastvisinc = (int32_t) totalclock + 32;
                    A_PlaySound(CHAINGUN_FIRE, pPlayer->i);
                    fi.shoot(pPlayer->i, TILE_CHAINGUN);
                    pPlayer->noise_radius = 16384;
                    madenoise(playerNum);
                    pPlayer->ammo_amount[MOTORCYCLE_WEAPON]--;
                    if (pPlayer->ammo_amount[MOTORCYCLE_WEAPON] <= 0)
                        *weaponFrame = 0;
                    else
                        P_CheckWeapon(pPlayer);
                }
                if ((*weaponFrame) == 2)
                {
                    pPlayer->q16ang += F16(4);
                }
                else if ((*weaponFrame) == 4)
                {
                    pPlayer->q16ang -= F16(4);
                }
                if ((*weaponFrame) > 4)
                    (*weaponFrame) = 1;
                if (!TEST_SYNC_KEY(playerBits, SK_FIRE))
                    (*weaponFrame) = 0;
                break;

            case BOAT_WEAPON__STATIC:
                if (!RRRA) break;
                if (*weaponFrame == 3)
                {
                    pPlayer->MotoSpeed -= 20;
                    pPlayer->ammo_amount[BOAT_WEAPON]--;
                    fi.shoot(pPlayer->i, TILE_RRTILE1790);
                }
                (*weaponFrame)++;
                if ((*weaponFrame) > 20)
                {
                    (*weaponFrame) = 0;
                    P_CheckWeapon(pPlayer);
                }
                if (pPlayer->ammo_amount[BOAT_WEAPON] <= 0)
                    (*weaponFrame) = 0;
                else
                    P_CheckWeapon(pPlayer);
                break;

            case FREEZE_WEAPON__STATIC:
                (*weaponFrame)++;
                if ((*weaponFrame) >= 7 && (*weaponFrame) <= 11)
                    fi.shoot(pPlayer->i, TILE_FIRELASER);

                if ((*weaponFrame) == 5)
                {
                    A_PlaySound(CAT_FIRE, pPlayer->i);
                    pPlayer->noise_radius = 2048;
                    madenoise(playerNum);
                }
                else if ((*weaponFrame) == 9)
                {
                    pPlayer->ammo_amount[FREEZE_WEAPON]--;
                    pPlayer->visibility = 0;
                    flashColor = 72 + (88 << 8) + (140 << 16);
                    lastvisinc = (int32_t) totalclock + 32;
                    P_CheckWeapon(pPlayer);
                }
                else if ((*weaponFrame) == 12)
                {
                    pPlayer->vel.x -= sintable[(fix16_to_int(pPlayer->q16ang) + 512) & 2047] << 4;
                    pPlayer->vel.y -= sintable[fix16_to_int(pPlayer->q16ang) & 2047] << 4;
                    pPlayer->q16horiz += F16(20);
                    pPlayer->recoil += 20;
                }
                if ((*weaponFrame) > 20)
                    (*weaponFrame) = 0;
                break;

            case TRIPBOMB_WEAPON__STATIC:
                if ((*weaponFrame) == 3)
                {
                    int Zvel;
                    int FwdVel;

                    if (playerNum == screenpeek)
                        pus = 1;
                    pPlayer->ammo_amount[TRIPBOMB_WEAPON]--;
                    pPlayer->gotweapon.Clear(TRIPBOMB_WEAPON);
                    if (pPlayer->on_ground && TEST_SYNC_KEY(playerBits, SK_CROUCH) && (!RRRA || !pPlayer->OnMotorcycle))
                    {
                        FwdVel = 15;
                        Zvel = (fix16_to_int(pPlayer->q16horiz + pPlayer->q16horizoff - F16(100)) * 20);
                    }
                    else
                    {
                        FwdVel = 32;
                        Zvel = -512 - (fix16_to_int(pPlayer->q16horiz + pPlayer->q16horizoff - F16(100)) * 20);
                    }

                    A_InsertSprite(pPlayer->cursectnum,
                                   pPlayer->pos.x+(sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047]>>6),
                                   pPlayer->pos.y+(sintable[fix16_to_int(pPlayer->q16ang)&2047]>>6),
                                   pPlayer->pos.z,TILE_TRIPBOMBSPRITE,-16,9,9,
                                   fix16_to_int(pPlayer->q16ang),FwdVel*2,Zvel,pPlayer->i,1);
                }
                (*weaponFrame)++;
                if ((*weaponFrame) > 20)
                {
                    (*weaponFrame) = 0;
                    P_CheckWeapon(pPlayer);
                }
                break;

            case BOWLINGBALL_WEAPON__STATIC:
                if ((*weaponFrame) == 30)
                {
                    pPlayer->ammo_amount[BOWLING_WEAPON]--;
                    A_PlaySound(354, pPlayer->i);
                    fi.shoot(pPlayer->i, TILE_BOWLINGBALL);
                    pPlayer->noise_radius = 1024;
                    madenoise(playerNum);
                }
                if ((*weaponFrame) < 30)
                    P_Thrust(pPlayer, 4);
                (*weaponFrame)++;
                if ((*weaponFrame) > 40)
                {
                    (*weaponFrame) = 0;
                    pPlayer->gotweapon.Clear(BOWLING_WEAPON);
                    P_CheckWeapon(pPlayer);
                }
                break;

            case KNEE_WEAPON__STATIC:
                if (++(*weaponFrame) == 3)
                    A_PlaySound(426, pPlayer->i);

                if ((*weaponFrame) == 12)
                {
                    fi.shoot(pPlayer->i, TILE_KNEE);
                    pPlayer->noise_radius = 1024;
                    madenoise(playerNum);
                }
                else if ((*weaponFrame) == 16)
                    (*weaponFrame) = 0;

                if (pPlayer->wantweaponfire >= 0)
                    P_CheckWeapon(pPlayer);
                break;

            case SLINGBLADE_WEAPON__STATIC:
                if (!RRRA) break;
                if (++(*weaponFrame) == 3)
                    A_PlaySound(252, pPlayer->i);

                if ((*weaponFrame) == 8)
                {
                    fi.shoot(pPlayer->i, TILE_SLINGBLADE);
                    pPlayer->noise_radius = 1024;
                    madenoise(playerNum);
                }
                else if ((*weaponFrame) == 16)
                    (*weaponFrame) = 0;

                if (pPlayer->wantweaponfire >= 0)
                    P_CheckWeapon(pPlayer);
                break;

            case RPG_WEAPON__STATIC:
                if (++(*weaponFrame) == 4)
                {
                    pPlayer->ammo_amount[CROSSBOW_WEAPON]--;
                    if (pPlayer->ammo_amount[DYNAMITE_WEAPON])
                        pPlayer->ammo_amount[DYNAMITE_WEAPON]--;
                    lastvisinc = (int32_t) totalclock + 32;
                    pPlayer->visibility = 0;
                    flashColor = 255+(95<<8);
                    fi.shoot(pPlayer->i, TILE_RPG);
                    pPlayer->noise_radius = 32768;
                    madenoise(playerNum);
                    P_CheckWeapon(pPlayer);
                }
                else if ((*weaponFrame) == 16)
                    A_PlaySound(450, pPlayer->i);
                else if ((*weaponFrame) == 34)
                    (*weaponFrame) = 0;
                break;

            case CHICKEN_WEAPON__STATIC:
                if (!RRRA) break;
                if (++(*weaponFrame) == 4)
                {
                    pPlayer->ammo_amount[CHICKEN_WEAPON]--;
                    lastvisinc = (int32_t) totalclock + 32;
                    pPlayer->visibility = 0;
                    flashColor = 255+(95<<8);
                    fi.shoot(pPlayer->i, TILE_RPG2);
                    pPlayer->noise_radius = 32768;
                    madenoise(playerNum);
                    P_CheckWeapon(pPlayer);
                }
                else if ((*weaponFrame) == 16)
                    A_PlaySound(450, pPlayer->i);
                else if ((*weaponFrame) == 34)
                    (*weaponFrame) = 0;
                break;
            }
        }
        else if (WW2GI)
        {
            if (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == HANDBOMB_WEAPON)
            {
                if (PWEAPON(playerNum, pPlayer->curr_weapon, HoldDelay) && ((*weaponFrame) == PWEAPON(playerNum, pPlayer->curr_weapon, FireDelay)) && TEST_SYNC_KEY(playerBits, SK_FIRE))
                {
                    pPlayer->rapid_fire_hold = 1;
                    return;
                }

                if (++(*weaponFrame) == PWEAPON(playerNum, pPlayer->curr_weapon, HoldDelay))
                {
                    pPlayer->ammo_amount[pPlayer->curr_weapon]--;

                    //int pipeBombType;
                    int pipeBombZvel;
                    int pipeBombFwdVel;

                    if (pPlayer->on_ground && TEST_SYNC_KEY(playerBits, SK_CROUCH))
                    {
                        pipeBombFwdVel = 15;
                        pipeBombZvel   = (fix16_to_int(pPlayer->q16horiz + pPlayer->q16horizoff - F16(100)) * 20);
                    }
                    else
                    {
                        pipeBombFwdVel = 140;
                        pipeBombZvel   = -512 - (fix16_to_int(pPlayer->q16horiz + pPlayer->q16horizoff - F16(100)) * 20);
                    }

                    int pipeSpriteNum = A_InsertSprite(pPlayer->cursectnum,
                                        pPlayer->pos.x+(sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047]>>6),
                                        pPlayer->pos.y+(sintable[fix16_to_int(pPlayer->q16ang)&2047]>>6),
                                        pPlayer->pos.z,PWEAPON(playerNum, pPlayer->curr_weapon, Shoots),-16,9,9,
                                        fix16_to_int(pPlayer->q16ang),(pipeBombFwdVel+(pPlayer->hbomb_hold_delay<<5)),pipeBombZvel,pPlayer->i,1);

                    int pipeLifeTime     = GetGameVar("GRENADE_LIFETIME", NAM_GRENADE_LIFETIME, -1, playerNum);
                    int pipeLifeVariance = GetGameVar("GRENADE_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, -1, playerNum);
                    sprite[pipeSpriteNum].extra = pipeLifeTime
                                        + mulscale14(krand2(), pipeLifeVariance)
                                        - pipeLifeVariance;

                    if (pipeBombFwdVel == 15)
                    {
                        sprite[pipeSpriteNum].yvel = 3;
                        sprite[pipeSpriteNum].z += ZOFFSET3;
                    }

                    if (hits(pPlayer->i) < 512)
                    {
                        sprite[pipeSpriteNum].ang += 1024;
                        sprite[pipeSpriteNum].zvel /= 3;
                        sprite[pipeSpriteNum].xvel /= 3;
                    }

                    pPlayer->hbomb_on = 1;
                }
                else if ((*weaponFrame) < PWEAPON(playerNum, pPlayer->curr_weapon, HoldDelay) && TEST_SYNC_KEY(playerBits, SK_FIRE))
                    pPlayer->hbomb_hold_delay++;
                else if ((*weaponFrame) > PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime))
                {
                    (*weaponFrame) = 0;
                    P_CheckWeapon(pPlayer);
                }
            }
            else if (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == HANDREMOTE_WEAPON)
            {
                if (++(*weaponFrame) == PWEAPON(playerNum, pPlayer->curr_weapon, FireDelay))
                {
                    if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_BOMB_TRIGGER)
                        pPlayer->hbomb_on = 0;

                    if (PWEAPON(playerNum, pPlayer->curr_weapon, Shoots) != 0)
                    {
                        if (!(PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_NOVISIBLE))
                        {
                            lastvisinc = (int32_t) totalclock+32;
                            pPlayer->visibility = 0;
                        }

                        P_SetWeaponGamevars(playerNum, pPlayer);
                        fi.shoot(pPlayer->i, PWEAPON(playerNum, pPlayer->curr_weapon, Shoots));
                    }
                }

                if ((*weaponFrame) >= PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime))
                {
                    (*weaponFrame) = 0;
                    if (pPlayer->ammo_amount[HANDBOMB_WEAPON] > 0)
                        P_AddWeapon(pPlayer, HANDBOMB_WEAPON);
                    else P_CheckWeapon(pPlayer);
                }
            }
            else
            {
                // the basic weapon...
                (*weaponFrame)++;

                if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_CHECKATRELOAD)
                {
                    if (*weaponFrame == PWEAPON(playerNum, pPlayer->curr_weapon, Reload))
                        P_CheckWeapon(pPlayer);
                }

                if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_STANDSTILL
                        && *weaponFrame < (PWEAPON(playerNum, pPlayer->curr_weapon, FireDelay)+1))
                {
                    pPlayer->pos.z = pPlayer->opos.z;
                    pPlayer->vel.z = 0;
                }

                if (*weaponFrame == PWEAPON(playerNum, pPlayer->curr_weapon, Sound2Time))
                    if (PWEAPON(playerNum, pPlayer->curr_weapon, Sound2Sound) > 0)
                        A_PlaySound(PWEAPON(playerNum, pPlayer->curr_weapon, Sound2Sound),pPlayer->i);

                if (*weaponFrame == PWEAPON(playerNum, pPlayer->curr_weapon, SpawnTime))
                    P_DoWeaponSpawn(playerNum);

                if (*weaponFrame == PWEAPON(playerNum, pPlayer->curr_weapon, FireDelay))
                    P_FireWeapon(playerNum);

                if (*weaponFrame > PWEAPON(playerNum, pPlayer->curr_weapon, FireDelay)
                         && *weaponFrame < PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime))
                {
                    if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_AUTOMATIC)
                    {
                        if (TEST_SYNC_KEY(playerBits, SK_FIRE) == 0)
                            *weaponFrame = PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime);
                        if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_FIREEVERYTHIRD)
                        {
                            if (((*(weaponFrame))%3) == 0)
                            {
                                P_FireWeapon(playerNum);
                                P_DoWeaponSpawn(playerNum);
                            }
                        }
                        if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_FIREEVERYOTHER)
                        {
                            P_FireWeapon(playerNum);
                            P_DoWeaponSpawn(playerNum);
                        }
                    }
                }
                else if (*weaponFrame >= PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime))
                {
                    if (PWEAPON(playerNum, pPlayer->curr_weapon, Reload) > PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime) && pPlayer->ammo_amount[pPlayer->curr_weapon] > 0
                        && PWEAPON(playerNum, pPlayer->curr_weapon, Clip) && pPlayer->ammo_amount[pPlayer->curr_weapon] % PWEAPON(playerNum, pPlayer->curr_weapon, Clip) == 0)
                    {
                        int const weaponReloadTime = PWEAPON(playerNum, pPlayer->curr_weapon, Reload)
                                                   - PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime);

                        if ((*weaponFrame) == (PWEAPON(playerNum, pPlayer->curr_weapon, TotalTime)+1))
                        {
                            A_PlaySound(EJECT_CLIP, pPlayer->i);
                        }
                        else if ((*weaponFrame) ==
                                    (PWEAPON(playerNum, pPlayer->curr_weapon, Reload) - (weaponReloadTime / 3)))
                        {
                            A_PlaySound(INSERT_CLIP, pPlayer->i);
                        }
                        if ((*weaponFrame) >= (PWEAPON(playerNum, pPlayer->curr_weapon, Reload)))
                        {
                            *weaponFrame       = 0;
                        }
                    }
                    else
                    {
                        if (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_AUTOMATIC)
                        {
                            if (TEST_SYNC_KEY(playerBits, SK_FIRE))
                            {
                                *weaponFrame =
                                (PWEAPON(playerNum, pPlayer->curr_weapon, Flags) & WEAPON_RANDOMRESTART) ? 1 + (krand2() & 3) : 1;
                            }
                            else *weaponFrame = 0;
                        }
                        else *weaponFrame = 0;
                    }
                }
            }
        }
        else
        {
            switch (DYNAMICWEAPONMAP(pPlayer->curr_weapon))
            {
            case HANDBOMB_WEAPON__STATIC:
                if ((*weaponFrame) == 6 && TEST_SYNC_KEY(playerBits, SK_FIRE))
                {
                    pPlayer->rapid_fire_hold = 1;
                    break;
                }

                if (++(*weaponFrame) == 12)
                {
                    pPlayer->ammo_amount[pPlayer->curr_weapon]--;

                    //if (numplayers < 2 || g_netServer)
                    {
                        int pipeBombZvel;
                        int pipeBombFwdVel;

                        if (pPlayer->on_ground && TEST_SYNC_KEY(playerBits, SK_CROUCH))
                        {
                            pipeBombFwdVel = 15;
                            pipeBombZvel   = (fix16_to_int(pPlayer->q16horiz + pPlayer->q16horizoff - F16(100)) * 20);
                        }
                        else
                        {
                            pipeBombFwdVel = 140;
                            pipeBombZvel   = -512 - (fix16_to_int(pPlayer->q16horiz + pPlayer->q16horizoff - F16(100)) * 20);
                        }

                        int pipeSpriteNum = A_InsertSprite(pPlayer->cursectnum,
                                           pPlayer->pos.x+(sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047]>>6),
                                           pPlayer->pos.y+(sintable[fix16_to_int(pPlayer->q16ang)&2047]>>6),
                                           pPlayer->pos.z,TILE_HEAVYHBOMB,-16,9,9,
                                           fix16_to_int(pPlayer->q16ang),(pipeBombFwdVel+(pPlayer->hbomb_hold_delay<<5)),pipeBombZvel,pPlayer->i,1);
                        
                        if (NAM)
                            sprite[pipeSpriteNum].extra = mulscale(krand2(), 30, 14)+90;

                        if (pipeBombFwdVel == 15)
                        {
                            sprite[pipeSpriteNum].yvel = 3;
                            sprite[pipeSpriteNum].z += ZOFFSET3;
                        }

                        if (hits(pPlayer->i) < 512)
                        {
                            sprite[pipeSpriteNum].ang += 1024;
                            sprite[pipeSpriteNum].zvel /= 3;
                            sprite[pipeSpriteNum].xvel /= 3;
                        }
                    }

                    pPlayer->hbomb_on = 1;
                }
                else if ((*weaponFrame) < 12 && TEST_SYNC_KEY(playerBits, SK_FIRE))
                    pPlayer->hbomb_hold_delay++;
                else if ((*weaponFrame) > 19)
                {
                    (*weaponFrame) = 0;
                    if (NAM)
                    {
                        // don't change to remote when in NAM: grenades are timed
                        P_CheckWeapon(pPlayer);
                    }
                    else
                    {
                        pPlayer->weapon_pos = WEAPON_POS_RAISE;
                        pPlayer->curr_weapon = HANDREMOTE_WEAPON;
                        pPlayer->last_weapon = -1;
                    }
                }
                break;

            case HANDREMOTE_WEAPON__STATIC:
                if (++(*weaponFrame) == 2)
                {
                    pPlayer->hbomb_on = 0;
                }

                if ((*weaponFrame) == 10)
                {
                    (*weaponFrame) = 0;
                    /// WHAT THE HELL DOES THIS DO....?????????????
                    int weapon = NAM ? TRIPBOMB_WEAPON : HANDBOMB_WEAPON;
                    if (pPlayer->ammo_amount[weapon] > 0)
                    {
                        P_AddWeapon(pPlayer, weapon);
                    }
                    else
                    {
                        P_CheckWeapon(pPlayer);
                    }
                }
                break;

            case PISTOL_WEAPON__STATIC:
                if ((*weaponFrame) == 1)
                {
                    fi.shoot(pPlayer->i, TILE_SHOTSPARK1);
                    A_PlaySound(PISTOL_FIRE, pPlayer->i);
                    lastvisinc = (int32_t) totalclock+32;
                    pPlayer->visibility = 0;
                    flashColor = 255+(95<<8);
                }
                else if ((*weaponFrame) == 2)
                {
                    fi.spawn(pPlayer->i, TILE_SHELL);
                }

                if (++(*weaponFrame) >= 5)
                {
                    if (pPlayer->ammo_amount[PISTOL_WEAPON] <= 0 || (pPlayer->ammo_amount[PISTOL_WEAPON]%(NAM ? 20 : 12)))
                    {
                        (*weaponFrame) = 0;
                        P_CheckWeapon(pPlayer);
                    }
                    else
                    {
                        switch ((*weaponFrame))
                        {
                        case 5:
                            A_PlaySound(EJECT_CLIP, pPlayer->i);
                            break;
                        case 8:
                            A_PlaySound(INSERT_CLIP, pPlayer->i);
                            break;
                        }
                    }
                }

                if ((*weaponFrame) == (NAM ? 50 : 27))
                {
                    (*weaponFrame) = 0;
                    P_CheckWeapon(pPlayer);
                }
                break;

            case SHOTGUN_WEAPON__STATIC:
                if (++(*weaponFrame) == 4)
                {
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);
                    fi.shoot(pPlayer->i, TILE_SHOTGUN);

                    pPlayer->ammo_amount[SHOTGUN_WEAPON]--;

                    A_PlaySound(SHOTGUN_FIRE, pPlayer->i);

                    lastvisinc = (int32_t) totalclock + 32;
                    pPlayer->visibility = 0;
                    flashColor = 255+(95<<8);
                }

                switch ((*weaponFrame))
                {
                case 13:
                    P_CheckWeapon(pPlayer);
                    break;
                case 15:
                    A_PlaySound(SHOTGUN_COCK, pPlayer->i);
                    break;
                case 17:
                case 20:
                    pPlayer->kickback_pic++;
                    break;
                case 24:
                    spriteNum = fi.spawn(pPlayer->i, TILE_SHOTGUNSHELL);
                    sprite[spriteNum].ang += 1024;
                    A_SetSprite(spriteNum, CLIPMASK0);
                    sprite[spriteNum].ang += 1024;
                    pPlayer->kickback_pic++;
                    break;
                case 31:
                    (*weaponFrame) = 0;
                    return;
                }
                break;

            case CHAINGUN_WEAPON__STATIC:
                if (++(*weaponFrame) <= 12)
                {
                    if (((*weaponFrame) % 3) == 0)
                    {
                        pPlayer->ammo_amount[CHAINGUN_WEAPON]--;

                        if (((*weaponFrame) % 3) == 0)
                        {
                            spriteNum = fi.spawn(pPlayer->i, TILE_SHELL);

                            sprite[spriteNum].ang += 1024;
                            sprite[spriteNum].ang &= 2047;
                            sprite[spriteNum].xvel += 32;
                            sprite[spriteNum].z += (3 << 8);
                            A_SetSprite(spriteNum, CLIPMASK0);
                        }

                        A_PlaySound(CHAINGUN_FIRE, pPlayer->i);
                        fi.shoot(pPlayer->i, TILE_CHAINGUN);
                        lastvisinc = (int32_t) totalclock + 32;
                        pPlayer->visibility = 0;
                        flashColor = 255+(95<<8);
                        P_CheckWeapon(pPlayer);

                        if (!TEST_SYNC_KEY(playerBits, SK_FIRE))
                        {
                            (*weaponFrame) = 0;
                            break;
                        }
                    }
                }
                else if ((*weaponFrame) > 10)
                {
                    if (TEST_SYNC_KEY(playerBits, SK_FIRE))
                    {
                        (*weaponFrame) = 1;
                    }
                    else
                    {
                        (*weaponFrame) = 0;
                    }
                }

                break;

            case GROW_WEAPON__STATIC:
                if ((!NAM && (*weaponFrame) > 3) || (NAM && ++(*weaponFrame) == 3))
                {
                    if (NAM)
                    {
                        (*weaponFrame)++;
                        if (pPlayer->ammo_amount[GROW_WEAPON] <= 1)
                            (*weaponFrame) = 0;
                    }
                    else
                        (*weaponFrame) = 0;
                    if (screenpeek == playerNum)
                    {
                        pus = 1;
                    }

                    pPlayer->ammo_amount[GROW_WEAPON]--;

                    fi.shoot(pPlayer->i, TILE_GROWSPARK);

                    pPlayer->visibility = 0;
                    flashColor = 216+(52<<8)+(20<<16);
                    lastvisinc = (int32_t) totalclock + 32;
                    P_CheckWeapon(pPlayer);
                }
                else if (!NAM)
                {
                    (*weaponFrame)++;
                }
                if (NAM && (*weaponFrame) > 30)
                {
                    // reload now...
                    (*weaponFrame) = 0;

                    pPlayer->visibility = 0;
                    flashColor = 216+(52<<8)+(20<<16);
                    lastvisinc = (int32_t) totalclock + 32;
                    P_CheckWeapon(pPlayer);
                    P_CheckWeapon(pPlayer);
                }
                break;

            case SHRINKER_WEAPON__STATIC:
                if ((!NAM && (*weaponFrame) > 10) || (NAM && (*weaponFrame) == 10))
                {
                    if (NAM)
                    {
                        // fire now, but wait for reload...
                        (*weaponFrame)++;
                    }
                    else
                        (*weaponFrame) = 0;

                    pPlayer->ammo_amount[SHRINKER_WEAPON]--;

                    fi.shoot(pPlayer->i, TILE_SHRINKER);

                    if (!NAM)
                    {
                        pPlayer->visibility = 0;
                        flashColor = 176+(252<<8)+(120<<16);
                        lastvisinc = (int32_t) totalclock + 32;
                        P_CheckWeapon(pPlayer);
                    }
                }
                else if (NAM && (*weaponFrame) > 30)
                {
                    (*weaponFrame) = 0;
                    pPlayer->visibility = 0;
                    flashColor = 176+(252<<8)+(120<<16);
                    lastvisinc = (int32_t) totalclock + 32;
                    P_CheckWeapon(pPlayer);
                }
                else
                {
                    (*weaponFrame)++;
                }
                break;

            case DEVISTATOR_WEAPON__STATIC:
                if ((*weaponFrame) > 0)
                {
                    if (++(*weaponFrame) & 1)
                    {
                        pPlayer->visibility = 0;
                        flashColor = 255+(95<<8);
                        lastvisinc = (int32_t) totalclock + 32;
                        fi.shoot(pPlayer->i, TILE_RPG);
                        pPlayer->ammo_amount[DEVISTATOR_WEAPON]--;
                        P_CheckWeapon(pPlayer);
                    }
                    if ((*weaponFrame) > 5)
                    {
                        (*weaponFrame) = 0;
                    }
                }
                break;

            case FREEZE_WEAPON__STATIC:
                if ((*weaponFrame) < 4)
                {
                    if (++(*weaponFrame) == 3)
                    {
                        pPlayer->ammo_amount[FREEZE_WEAPON]--;
                        pPlayer->visibility = 0;
                        flashColor = 72+(88<<8)+(140<<16);
                        lastvisinc = (int32_t) totalclock + 32;
                        fi.shoot(pPlayer->i, TILE_FREEZEBLAST);
                        P_CheckWeapon(pPlayer);
                    }
                    if (sprite[pPlayer->i].xrepeat < 32)
                    {
                        (*weaponFrame) = 0;
                    }
                }
                else
                {
                    if (TEST_SYNC_KEY(playerBits, SK_FIRE))
                    {
                        (*weaponFrame) = 1;
                        A_PlaySound(CAT_FIRE, pPlayer->i);
                    }
                    else
                    {
                        (*weaponFrame) = 0;
                    }
                }
                break;

            case TRIPBOMB_WEAPON__STATIC:
                if ((*weaponFrame) < 4)
                {
                    pPlayer->pos.z = pPlayer->opos.z;
                    pPlayer->vel.z = 0;
                    if ((*weaponFrame) == 3)
                    {
                        fi.shoot(pPlayer->i, TILE_HANDHOLDINGLASER);
                    }
                }
                if ((*weaponFrame) == 16)
                {
                    (*weaponFrame) = 0;
                    P_CheckWeapon(pPlayer);
                    pPlayer->weapon_pos = WEAPON_POS_LOWER;
                }
                else
                {
                    (*weaponFrame)++;
                }
                break;

            case KNEE_WEAPON__STATIC:
                if (++(*weaponFrame) == 7)
                {
                    fi.shoot(pPlayer->i, TILE_KNEE);
                }
                else if ((*weaponFrame) == 14)
                {
                    if (TEST_SYNC_KEY(playerBits, SK_FIRE))
                    {
                        (*weaponFrame) = 1+(krand2()&3);
                    }
                    else
                    {
                        (*weaponFrame) = 0;
                    }
                }

                if (pPlayer->wantweaponfire >= 0)
                {
                    P_CheckWeapon(pPlayer);
                }
                break;

            case RPG_WEAPON__STATIC:
                if (++(*weaponFrame) == 4)
                {
                    pPlayer->ammo_amount[RPG_WEAPON]--;
                    lastvisinc = (int32_t) totalclock + 32;
                    pPlayer->visibility = 0;
                    flashColor = 255+(95<<8);
                    fi.shoot(pPlayer->i, TILE_RPG);
                    P_CheckWeapon(pPlayer);
                }
                else if ((*weaponFrame) == 20)
                {
                    (*weaponFrame) = 0;
                }
                break;
            }
        }
#ifdef POLYMER
        if (flashColor)
        {
            spritetype *s = &sprite[pPlayer->i];
            int32_t     x = ((sintable[(s->ang + 512) & 2047]) >> 7), y = ((sintable[(s->ang) & 2047]) >> 7);

            s->x += x;
            s->y += y;
            G_AddGameLight(0, pPlayer->i, PHEIGHT, 8192, flashColor, PR_LIGHT_PRIO_MAX_GAME);
            actor[pPlayer->i].lightcount = 2;
            s->x -= x;
            s->y -= y;
        }
#endif  // POLYMER
    }
}

void P_EndLevel(void)
{
    for (bssize_t TRAVERSE_CONNECT(playerNum))
        g_player[playerNum].ps->gm = MODE_EOL;

    if (ud.from_bonus)
    {
        ud.level_number   = ud.from_bonus;
        m_level_number = ud.level_number;
        ud.from_bonus     = 0;
    }
    else
    {
        ud.level_number   = (++ud.level_number < MAXLEVELS) ? ud.level_number : 0;
        m_level_number = ud.level_number;
    }
}

static int P_DoFist(DukePlayer_t *pPlayer)
{
    // the fist punching TILE_NUKEBUTTON

    if (++(pPlayer->fist_incs) == 28)
    {
        if (ud.recstat == 1)
            G_CloseDemoWrite();

        S_PlaySound(PIPEBOMB_EXPLODE);
        P_PalFrom(pPlayer, 48, 64, 64, 64);
    }

    if (pPlayer->fist_incs > 42)
    {
        if (pPlayer->buttonpalette && ud.from_bonus == 0)
        {
            for (bssize_t TRAVERSE_CONNECT(playerNum))
                g_player[playerNum].ps->gm = MODE_EOL;

            ud.from_bonus = ud.level_number + 1;

            if ((unsigned)ud.secretlevel <= MAXLEVELS)
                ud.level_number = ud.secretlevel - 1;

            m_level_number = ud.level_number;
        }
        else
            P_EndLevel();

        pPlayer->fist_incs = 0;

        return 1;
    }

    return 0;
}

#ifdef YAX_ENABLE
void getzsofslope_player(int sectNum, int playerX, int playerY, int32_t *pCeilZ, int32_t *pFloorZ)
{
    int didCeiling = 0;

    if ((sector[sectNum].ceilingstat & 512) == 0)
    {
        int const neighborSect = yax_getneighborsect(playerX, playerY, sectNum, YAX_CEILING);

        if (neighborSect >= 0)
        {
            *pCeilZ    = getceilzofslope(neighborSect, playerX, playerY);
            didCeiling = 1;
        }
    }

    int didFloor   = 0;

    if ((sector[sectNum].floorstat & 512) == 0)
    {
        int const neighborSect = yax_getneighborsect(playerX, playerY, sectNum, YAX_FLOOR);

        if (neighborSect >= 0)
        {
            *pFloorZ = getflorzofslope(neighborSect, playerX, playerY);
            didFloor = 1;
        }
    }

    if (!didCeiling || !didFloor)
    {
        int32_t ceilingZ, floorZ;
        getzsofslope(sectNum, playerX, playerY, &ceilingZ, &floorZ);

        if (!didCeiling)
            *pCeilZ = ceilingZ;

        if (!didFloor)
            *pFloorZ = floorZ;
    }
}
#endif

void P_UpdatePosWhenViewingCam(DukePlayer_t *pPlayer)
{
    int const newOwner      = pPlayer->newowner;
    pPlayer->pos            = *(vec3_t *)&sprite[newOwner];
    pPlayer->q16ang           = fix16_from_int(SA(newOwner));
    pPlayer->vel.x          = 0;
    pPlayer->vel.y          = 0;
    sprite[pPlayer->i].xvel = 0;
    pPlayer->look_ang       = 0;
    pPlayer->rotscrnang     = 0;
}

static void P_DoWater(int const playerNum, int const playerBits, int const floorZ, int const ceilZ)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

    // under water
    pPlayer->pycount        += 32;
    pPlayer->pycount        &= 2047;
    pPlayer->jumping_counter = 0;
    pPlayer->pyoff           = sintable[pPlayer->pycount] >> 7;

    if (!A_CheckSoundPlaying(pPlayer->i, DUKE_UNDERWATER))
        A_PlaySound(DUKE_UNDERWATER, pPlayer->i);

    if (TEST_SYNC_KEY(playerBits, SK_JUMP) && (!RRRA || !pPlayer->OnMotorcycle))
    {
        pPlayer->vel.z = max(min(-348, pPlayer->vel.z - 348), -(256 * 6));
    }
    else if ((TEST_SYNC_KEY(playerBits, SK_CROUCH) && (!RRRA || !pPlayer->OnMotorcycle))
        || (RRRA && pPlayer->OnMotorcycle))
    {
        pPlayer->vel.z = min(max(348, pPlayer->vel.z + 348), (256 * 6));
    }
    else
    {
        // normal view
        if (pPlayer->vel.z < 0)
            pPlayer->vel.z = min(0, pPlayer->vel.z + 256);

        if (pPlayer->vel.z > 0)
            pPlayer->vel.z = max(0, pPlayer->vel.z - 256);
    }

    if (pPlayer->vel.z > 2048)
        pPlayer->vel.z >>= 1;

    pPlayer->pos.z += pPlayer->vel.z;

    if (pPlayer->pos.z > (floorZ-(15<<8)))
        pPlayer->pos.z += ((floorZ-(15<<8))-pPlayer->pos.z)>>1;

    if (pPlayer->pos.z < ceilZ+ZOFFSET6)
    {
        pPlayer->pos.z = ceilZ+ZOFFSET6;
        pPlayer->vel.z = 0;
    }

    if (pPlayer->scuba_on && (krand2()&255) < 8)
    {
        int const spriteNum = fi.spawn(pPlayer->i, TILE_WATERBUBBLE);
        int const q16ang      = fix16_to_int(pPlayer->q16ang);

        sprite[spriteNum].x      += sintable[(q16ang + 512 + 64 - (g_globalRandom & 128)+(RR ? 128 : 0)) & 2047] >> 6;
        sprite[spriteNum].y      += sintable[(q16ang + 64 - (g_globalRandom & 128)+(RR ? 128 : 0)) & 2047] >> 6;
        sprite[spriteNum].xrepeat = 3;
        sprite[spriteNum].yrepeat = 2;
        sprite[spriteNum].z       = pPlayer->pos.z + ZOFFSET3;
        if (RR)
            sprite[spriteNum].cstat = 514;
    }
}
static void P_DoJetpack(int const playerNum, int const playerBits, int const playerShrunk, int const sectorLotag, int const floorZ)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

    pPlayer->on_ground       = 0;
    pPlayer->jumping_counter = 0;
    pPlayer->hard_landing    = 0;
    pPlayer->falling_counter = 0;
    pPlayer->pycount        += 32;
    pPlayer->pycount        &= 2047;
    pPlayer->pyoff           = sintable[pPlayer->pycount] >> 7;

    if (pPlayer->jetpack_on < 11)
    {
        pPlayer->jetpack_on++;
        pPlayer->pos.z -= (pPlayer->jetpack_on<<7); //Goin up
    }
    else if (pPlayer->jetpack_on == 11 && !A_CheckSoundPlaying(pPlayer->i, DUKE_JETPACK_IDLE))
        A_PlaySound(DUKE_JETPACK_IDLE, pPlayer->i);

    int const zAdjust = playerShrunk ? 512 : 2048;

    if (TEST_SYNC_KEY(playerBits, SK_JUMP))  // jumping, flying up
    {
        if (VM_OnEvent(EVENT_SOARUP, pPlayer->i, playerNum) == 0)
        {
            pPlayer->pos.z -= zAdjust;
            pPlayer->crack_time = 777;
        }
    }

    if (TEST_SYNC_KEY(playerBits, SK_CROUCH))  // crouching, flying down
    {
        if (VM_OnEvent(EVENT_SOARDOWN, pPlayer->i, playerNum) == 0)
        {
            pPlayer->pos.z += zAdjust;
            pPlayer->crack_time = 777;
        }
    }

    int const Zdiff = (playerShrunk == 0 && (sectorLotag == 0 || sectorLotag == ST_2_UNDERWATER)) ? 32 : 16;

    if (sectorLotag != ST_2_UNDERWATER && pPlayer->scuba_on == 1)
        pPlayer->scuba_on = 0;

    if (pPlayer->pos.z > (floorZ - (Zdiff << 8)))
        pPlayer->pos.z += ((floorZ - (Zdiff << 8)) - pPlayer->pos.z) >> 1;

    if (pPlayer->pos.z < (actor[pPlayer->i].ceilingz + (18 << 8)))
        pPlayer->pos.z = actor[pPlayer->i].ceilingz + (18 << 8);
}

static void P_Dead(int const playerNum, int const sectorLotag, int const floorZ, int const ceilZ)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;
    spritetype *const   pSprite = &sprite[pPlayer->i];

    if (ud.recstat == 1 && (!g_netServer && ud.multimode < 2))
        G_CloseDemoWrite();

    if (/*(numplayers < 2 || g_netServer) && */pPlayer->dead_flag == 0)
        P_FragPlayer(playerNum);

    if (sectorLotag == ST_2_UNDERWATER)
    {
        if (pPlayer->on_warping_sector == 0)
        {
            if (klabs(pPlayer->pos.z-floorZ) >(PHEIGHT>>1))
                pPlayer->pos.z += 348;
        }
        else
        {
            pSprite->z -= 512;
            pSprite->zvel = -348;
        }

        clipmove((vec3_t *) pPlayer, &pPlayer->cursectnum,
            0, 0, 164, (4L<<8), (4L<<8), CLIPMASK0);
        //                        p->bobcounter += 32;
    }

    Bmemcpy(&pPlayer->opos, &pPlayer->pos, sizeof(vec3_t));
    pPlayer->opyoff = pPlayer->pyoff;

    pPlayer->q16horiz = F16(100);
    pPlayer->q16horizoff = 0;

    updatesector(pPlayer->pos.x, pPlayer->pos.y, &pPlayer->cursectnum);

    pushmove((vec3_t *) pPlayer, &pPlayer->cursectnum, 128L, (4L<<8), (20L<<8), CLIPMASK0);

    if (floorZ > ceilZ + ZOFFSET2 && pSprite->pal != 1)
        pPlayer->rotscrnang = (pPlayer->dead_flag + ((floorZ+pPlayer->pos.z)>>7))&2047;

    pPlayer->on_warping_sector = 0;
}


static void P_HandlePal(DukePlayer_t *const pPlayer)
{
    pPlayer->pals.f--;
}

void P_ProcessInput(int playerNum)
{
    auto &thisPlayer = g_player[playerNum];

    thisPlayer.horizAngleAdjust = 0;
    thisPlayer.horizSkew = 0;

    if (thisPlayer.playerquitflag == 0)
        return;

    auto const pPlayer = thisPlayer.ps;
    auto const pSprite = &sprite[pPlayer->i];

    ++pPlayer->player_par;

    uint32_t playerBits = thisPlayer.input->bits;

    if (RR)
    {
        if (RRRA)
            g_canSeePlayer = 1;
        else
            g_canSeePlayer = playerNum;
    }

    if (pPlayer->cheat_phase > 0)
        playerBits = 0;

    if (RRRA)
    {
        if (pPlayer->OnMotorcycle && pSprite->extra > 0)
        {
            int var64, var68, var6c, var74, var7c;
            int16_t var84;
            if (pPlayer->MotoSpeed < 0)
                pPlayer->MotoSpeed = 0;
            if (TEST_SYNC_KEY(playerBits, SK_CROUCH))
            {
                var64 = 1;
                playerBits &= ~(1<<SK_CROUCH);
            }
            else
                var64 = 0;

            if (TEST_SYNC_KEY(playerBits, SK_JUMP))
            {
                var68 = 1;
                playerBits &= ~(1<< SK_JUMP);
                if (pPlayer->on_ground)
                {
                    if (pPlayer->MotoSpeed == 0 && var64)
                    {
                        if (!A_CheckSoundPlaying(pPlayer->i, 187))
                            A_PlaySound(187,pPlayer->i);
                    }
                    else if (pPlayer->MotoSpeed == 0 && !A_CheckSoundPlaying(pPlayer->i, 214))
                    {
                        if (A_CheckSoundPlaying(pPlayer->i, 187))
                            S_StopEnvSound(187, pPlayer->i);
                        A_PlaySound(214,pPlayer->i);
                    }
                    else if (pPlayer->MotoSpeed >= 50 && !A_CheckSoundPlaying(pPlayer->i, 188))
                    {
                        A_PlaySound(188,pPlayer->i);
                    }
                    else if (!A_CheckSoundPlaying(pPlayer->i, 188) && !A_CheckSoundPlaying(pPlayer->i, 214))
                    {
                        A_PlaySound(188,pPlayer->i);
                    }
                }
            }
            else
            {
                var68 = 0;
                if (A_CheckSoundPlaying(pPlayer->i, 214))
                {
                    S_StopEnvSound(214, pPlayer->i);
                    if (!A_CheckSoundPlaying(pPlayer->i, 189))
                        A_PlaySound(189,pPlayer->i);
                }
                if (A_CheckSoundPlaying(pPlayer->i, 188))
                {
                    S_StopEnvSound(188, pPlayer->i);
                    if (!A_CheckSoundPlaying(pPlayer->i, 189))
                        A_PlaySound(189, pPlayer->i);
                }
                if (!A_CheckSoundPlaying(pPlayer->i, 189) && !A_CheckSoundPlaying(pPlayer->i, 187))
                    A_PlaySound(187,pPlayer->i);
            }
            if (TEST_SYNC_KEY(playerBits, SK_AIM_UP))
            {
                var6c = 1;
                playerBits &= ~(1<<SK_AIM_UP);
            }
            else
                var6c = 0;
            if (TEST_SYNC_KEY(playerBits, SK_AIM_DOWN))
            {
                var74 = 1;
                playerBits &= ~(1<<SK_AIM_DOWN);
            }
            else
            {
                var74 = 0;
            }
            if (TEST_SYNC_KEY(playerBits, SK_LOOK_LEFT))
            {
                var7c = 1;
                playerBits &= ~(1<<SK_LOOK_LEFT);
            }
            else
            {
                var7c = 0;
            }
            if (pPlayer->drink_amt > 88 && pPlayer->moto_drink == 0)
            {
                var84 = krand2() & 63;
                if (var84 == 1)
                    pPlayer->moto_drink = -10;
                else if (var84 == 2)
                    pPlayer->moto_drink = 10;
            }
            else if (pPlayer->drink_amt > 99 && pPlayer->moto_drink == 0)
            {
                var84 = krand2() & 31;
                if (var84 == 1)
                    pPlayer->moto_drink = -20;
                else if (var84 == 2)
                    pPlayer->moto_drink = 20;
            }
            if (pPlayer->on_ground == 1)
            {
                if (var64 && pPlayer->MotoSpeed > 0)
                {
                    if (pPlayer->moto_on_oil)
                        pPlayer->MotoSpeed -= 2;
                    else
                        pPlayer->MotoSpeed -= 4;
                    if (pPlayer->MotoSpeed < 0)
                        pPlayer->MotoSpeed = 0;
                    pPlayer->moto_bump_target = -30;
                    pPlayer->moto_do_bump = 1;
                }
                else if (var68 && !var64)
                {
                    if (pPlayer->MotoSpeed < 40)
                    {
                        pPlayer->moto_bump_target = 70;
                        pPlayer->moto_bump_fast = 1;
                    }
                    pPlayer->MotoSpeed += 2;
                    if (pPlayer->MotoSpeed > 120)
                        pPlayer->MotoSpeed = 120;
                    if (!pPlayer->not_on_water)
                        if (pPlayer->MotoSpeed > 80)
                            pPlayer->MotoSpeed = 80;
                }
                else if (pPlayer->MotoSpeed > 0)
                    pPlayer->MotoSpeed--;
                if (pPlayer->moto_do_bump && (!var64 || pPlayer->MotoSpeed == 0))
                {
                    pPlayer->moto_bump_target = 0;
                    pPlayer->moto_do_bump = 0;
                }
                if (var6c && pPlayer->MotoSpeed <= 0 && !var64)
                {
                    int var88;
                    pPlayer->MotoSpeed = -15;
                    var88 = var7c;
                    var7c = var74;
                    var74 = var88;
                }
            }
            if (pPlayer->MotoSpeed != 0 && pPlayer->on_ground == 1)
            {
                if (!pPlayer->moto_bump)
                    if ((krand2() & 3) == 2)
                        pPlayer->moto_bump_target = (pPlayer->MotoSpeed>>4)*((krand2()&7)-4);
                if (var74 || pPlayer->moto_drink < 0)
                {
                    if (pPlayer->moto_drink < 0)
                        pPlayer->moto_drink++;
                }
                else if (var7c || pPlayer->moto_drink > 0)
                {
                    if (pPlayer->moto_drink > 0)
                        pPlayer->moto_drink--;
                }
            }
            if (pPlayer->moto_turb)
            {
                if (pPlayer->moto_turb <= 1)
                {
                    pPlayer->q16horiz = F16(100);
                    pPlayer->moto_turb = 0;
                    pPlayer->moto_bump_target = 0;
                    pPlayer->moto_bump = 0;
                }
                else
                {
                    pPlayer->q16horiz = F16(100+((krand2()&15)-7));
                    pPlayer->moto_turb--;
                    pPlayer->moto_drink = (krand2()&3)-2;
                }
            }
            else if (pPlayer->moto_bump_target > pPlayer->moto_bump)
            {
                if (pPlayer->moto_bump_fast)
                    pPlayer->moto_bump += 6;
                else
                    pPlayer->moto_bump++;
                if (pPlayer->moto_bump_target < pPlayer->moto_bump)
                    pPlayer->moto_bump = pPlayer->moto_bump_target;
                pPlayer->q16horiz = F16(100+pPlayer->moto_bump/3);
            }
            else if (pPlayer->moto_bump_target < pPlayer->moto_bump)
            {
                if (pPlayer->moto_bump_fast)
                    pPlayer->moto_bump -= 6;
                else
                    pPlayer->moto_bump--;
                if (pPlayer->moto_bump_target > pPlayer->moto_bump)
                    pPlayer->moto_bump = pPlayer->moto_bump_target;
                pPlayer->q16horiz = F16(100+pPlayer->moto_bump/3);
            }
            else
            {
                pPlayer->moto_bump_target = 0;
                pPlayer->moto_bump_fast = 0;
            }
            if (pPlayer->MotoSpeed >= 20 && pPlayer->on_ground == 1 && (var74 || var7c))
            {
                short var8c, var90, var94, var98;
                var8c = pPlayer->MotoSpeed;
                var90 = fix16_to_int(pPlayer->q16ang);
                if (var74)
                    var94 = -10;
                else
                    var94 = 10;
                if (var94 < 0)
                    var98 = 350;
                else
                    var98 = -350;
                if (pPlayer->moto_on_mud || pPlayer->moto_on_oil || !pPlayer->not_on_water)
                {
                    if (pPlayer->moto_on_oil)
                        var8c <<= 3;
                    else
                        var8c <<= 2;
                    if (pPlayer->moto_do_bump)
                    {
                        pPlayer->vel.x += (var8c>>5)*(sintable[(var94*-51+var90+512)&2047]<<4);
                        pPlayer->vel.y += (var8c>>5)*(sintable[(var94*-51+var90)&2047]<<4);
                        pPlayer->q16ang = F16((var90-(var98>>2))&2047);
                    }
                    else
                    {
                        pPlayer->vel.x += (var8c>>7)*(sintable[(var94*-51+var90+512)&2047]<<4);
                        pPlayer->vel.y += (var8c>>7)*(sintable[(var94*-51+var90)&2047]<<4);
                        pPlayer->q16ang = F16((var90-(var98>>6))&2047);
                    }
                    pPlayer->moto_on_mud = 0;
                    pPlayer->moto_on_oil = 0;
                }
                else
                {
                    if (pPlayer->moto_do_bump)
                    {
                        pPlayer->vel.x += (var8c >> 5)*(sintable[(var94*-51 + var90 + 512) & 2047] << 4);
                        pPlayer->vel.y += (var8c>>5)*(sintable[(var94*-51+var90)&2047]<<4);
                        pPlayer->q16ang = F16((var90-(var98>>4))&2047);
                        if (!A_CheckSoundPlaying(pPlayer->i, 220))
                            A_PlaySound(220,pPlayer->i);
                    }
                    else
                    {
                        pPlayer->vel.x += (var8c >> 7)*(sintable[(var94*-51 + var90 + 512) & 2047] << 4);
                        pPlayer->vel.y += (var8c>>7)*(sintable[(var94*-51+var90)&2047]<<4);
                        pPlayer->q16ang = F16((var90-(var98>>7))&2047);
                    }
                }
            }
            else if (pPlayer->MotoSpeed >= 20 && pPlayer->on_ground == 1 && (pPlayer->moto_on_mud || pPlayer->moto_on_oil))
            {
                short var9c, vara0, vara4 = 0;
                var9c = pPlayer->MotoSpeed;
                vara0 = fix16_to_int(pPlayer->q16ang);
                var84 = krand2()&1;
                if (var84 == 0)
                    vara4 = -10;
                else if (var84 == 1)
                    vara4 = 10;
                if (pPlayer->moto_on_oil)
                    var9c *= 10;
                else
                    var9c *= 5;
                pPlayer->vel.x += (var9c>>7)*(sintable[(vara4*-51+vara0+512)&2047]<<4);
                pPlayer->vel.y += (var9c>>7)*(sintable[(vara4*-51+vara0)&2047]<<4);
            }
            pPlayer->moto_on_mud = 0;
            pPlayer->moto_on_oil = 0;
        }
        else if (pPlayer->OnBoat && pSprite->extra > 0)
        {
            int vara8, varac, varb0, varb4, varbc, varc4;
            int16_t varcc;
            if (pPlayer->not_on_water)
            {
                if (pPlayer->MotoSpeed > 0)
                {
                    if (!A_CheckSoundPlaying(pPlayer->i, 88))
                        A_PlaySound(88,pPlayer->i);
                }
                else
                {
                    if (!A_CheckSoundPlaying(pPlayer->i, 87))
                        A_PlaySound(87,pPlayer->i);
                }
            }
            if (pPlayer->MotoSpeed < 0)
                pPlayer->MotoSpeed = 0;
            if (TEST_SYNC_KEY(playerBits, SK_CROUCH) && TEST_SYNC_KEY(playerBits, SK_JUMP))
            {
                vara8 = 1;
                varac = 0;
                playerBits &= ~(1<<SK_JUMP);
                varb0 = 0;
                playerBits &= ~(1<<SK_CROUCH);
            }
            else
                vara8 = 0;
            if (TEST_SYNC_KEY(playerBits, SK_JUMP))
            {
                varac = 1;
                playerBits &= ~(1<<SK_JUMP);
                if (pPlayer->MotoSpeed == 0 && !A_CheckSoundPlaying(pPlayer->i, 89))
                {
                    if (A_CheckSoundPlaying(pPlayer->i, 87))
                        S_StopEnvSound(pPlayer->i, 87);
                    A_PlaySound(89,pPlayer->i);
                }
                else if (pPlayer->MotoSpeed >= 50 && !A_CheckSoundPlaying(pPlayer->i, 88))
                    A_PlaySound(88,pPlayer->i);
                else if (!A_CheckSoundPlaying(pPlayer->i, 88) && !A_CheckSoundPlaying(pPlayer->i, 89))
                    A_PlaySound(88,pPlayer->i);
            }
            else
            {
                varac = 0;
                if (A_CheckSoundPlaying(pPlayer->i, 89))
                {
                    S_StopEnvSound(pPlayer->i, 89);
                    if (!A_CheckSoundPlaying(pPlayer->i, 90))
                        A_PlaySound(90,pPlayer->i);
                }
                if (A_CheckSoundPlaying(pPlayer->i, 88))
                {
                    S_StopEnvSound(pPlayer->i, 88);
                    if (!A_CheckSoundPlaying(pPlayer->i, 90))
                        A_PlaySound(90,pPlayer->i);
                }
                if (!A_CheckSoundPlaying(pPlayer->i, 90) && !A_CheckSoundPlaying(pPlayer->i, 87))
                    A_PlaySound(87,pPlayer->i);
            }
            if (TEST_SYNC_KEY(playerBits, SK_CROUCH))
            {
                varb0 = 1;
                playerBits &= ~(1<<SK_CROUCH);
            }
            else
                varb0 = 0;
            if (TEST_SYNC_KEY(playerBits, SK_AIM_UP))
            {
                varb4 = 1;
                playerBits &= ~(1<<SK_AIM_UP);
            }
            else varb4 = 0;
            if (TEST_SYNC_KEY(playerBits, SK_AIM_DOWN))
            {
                varbc = 1;
                playerBits &= ~(1<<SK_AIM_DOWN);
                if (!A_CheckSoundPlaying(pPlayer->i, 91) && pPlayer->MotoSpeed > 30 && !pPlayer->not_on_water)
                    A_PlaySound(91,pPlayer->i);
            }
            else
            {
                varbc = 0;
            }
            if (TEST_SYNC_KEY(playerBits, SK_LOOK_LEFT))
            {
                varc4 = 1;
                playerBits &= ~(1<< SK_LOOK_LEFT);
                if (!A_CheckSoundPlaying(pPlayer->i, 91) && pPlayer->MotoSpeed > 30 && !pPlayer->not_on_water)
                    A_PlaySound(91,pPlayer->i);
            }
            else
            {
                varc4 = 0;
            }
            if (!pPlayer->not_on_water)
            {
                if (pPlayer->drink_amt > 88 && pPlayer->moto_drink == 0)
                {
                    varcc = krand2() & 63;
                    if (varcc == 1)
                        pPlayer->moto_drink = -10;
                    else if (varcc == 2)
                        pPlayer->moto_drink = 10;
                }
                else if (pPlayer->drink_amt > 99 && pPlayer->moto_drink == 0)
                {
                    varcc = krand2() & 31;
                    if (varcc == 1)
                        pPlayer->moto_drink = -20;
                    else if (varcc == 2)
                        pPlayer->moto_drink = 20;
                }
            }
            if (pPlayer->on_ground == 1)
            {
                if (vara8)
                {
                    if (pPlayer->MotoSpeed <= 25)
                    {
                        pPlayer->MotoSpeed++;
                        if (!A_CheckSoundPlaying(pPlayer->i, 182))
                            A_PlaySound(182, pPlayer->i);
                    }
                    else
                    {
                        pPlayer->MotoSpeed -= 2;
                        if (pPlayer->MotoSpeed < 0)
                            pPlayer->MotoSpeed = 0;
                        pPlayer->moto_bump_target = 30;
                        pPlayer->moto_do_bump = 1;
                    }
                }
                else if (varb0 && pPlayer->MotoSpeed > 0)
                {
                    pPlayer->MotoSpeed -= 2;
                    if (pPlayer->MotoSpeed < 0)
                        pPlayer->MotoSpeed = 0;
                    pPlayer->moto_bump_target = 30;
                    pPlayer->moto_do_bump = 1;
                }
                else if (varac)
                {
                    if (pPlayer->MotoSpeed < 40)
                        if (!pPlayer->not_on_water)
                        {
                            pPlayer->moto_bump_target = -30;
                            pPlayer->moto_bump_fast = 1;
                        }
                    pPlayer->MotoSpeed++;
                    if (pPlayer->MotoSpeed > 120)
                        pPlayer->MotoSpeed = 120;
                }
                else if (pPlayer->MotoSpeed > 0)
                    pPlayer->MotoSpeed--;
                if (pPlayer->moto_do_bump && (!varb0 || pPlayer->MotoSpeed == 0))
                {
                    pPlayer->moto_bump_target = 0;
                    pPlayer->moto_do_bump = 0;
                }
                if (varb4 && pPlayer->MotoSpeed == 0 && !varb0)
                {
                    int vard0;
                    if (!pPlayer->not_on_water)
                        pPlayer->MotoSpeed = -25;
                    else
                        pPlayer->MotoSpeed = -20;
                    vard0 = varc4;
                    varc4 = varbc;
                    varbc = vard0;
                }
            }
            if (pPlayer->MotoSpeed != 0 && pPlayer->on_ground == 1)
            {
                if (!pPlayer->moto_bump)
                    if ((krand2() & 15) == 14)
                        pPlayer->moto_bump_target = (pPlayer->MotoSpeed>>4)*((krand2()&3)-2);
                if (varbc || pPlayer->moto_drink < 0)
                {
                    if (pPlayer->moto_drink < 0)
                        pPlayer->moto_drink++;
                }
                else if (varc4 || pPlayer->moto_drink > 0)
                {
                    if (pPlayer->moto_drink > 0)
                        pPlayer->moto_drink--;
                }
            }
            if (pPlayer->moto_turb)
            {
                if (pPlayer->moto_turb <= 1)
                {
                    pPlayer->q16horiz = F16(100);
                    pPlayer->moto_turb = 0;
                    pPlayer->moto_bump_target = 0;
                    pPlayer->moto_bump = 0;
                }
                else
                {
                    pPlayer->q16horiz = F16(100+((krand2()&15)-7));
                    pPlayer->moto_turb--;
                    pPlayer->moto_drink = (krand2()&3)-2;
                }
            }
            else if (pPlayer->moto_bump_target > pPlayer->moto_bump)
            {
                if (pPlayer->moto_bump_fast)
                    pPlayer->moto_bump += 6;
                else
                    pPlayer->moto_bump++;
                if (pPlayer->moto_bump_target < pPlayer->moto_bump)
                    pPlayer->moto_bump = pPlayer->moto_bump_target;
                pPlayer->q16horiz = F16(100+pPlayer->moto_bump/3);
            }
            else if (pPlayer->moto_bump_target < pPlayer->moto_bump)
            {
                if (pPlayer->moto_bump_fast)
                    pPlayer->moto_bump -= 6;
                else
                    pPlayer->moto_bump--;
                if (pPlayer->moto_bump_target > pPlayer->moto_bump)
                    pPlayer->moto_bump = pPlayer->moto_bump_target;
                pPlayer->q16horiz = F16(100+pPlayer->moto_bump/3);
            }
            else
            {
                pPlayer->moto_bump_target = 0;
                pPlayer->moto_bump_fast = 0;
            }
            if (pPlayer->MotoSpeed > 0 && pPlayer->on_ground == 1 && (varbc || varc4))
            {
                short vard4, vard8, vardc, vare0;
                vard4 = pPlayer->MotoSpeed;
                vard8 = fix16_to_int(pPlayer->q16ang);
                if (varbc)
                    vardc = -10;
                else
                    vardc = 10;
                if (vardc < 0)
                    vare0 = 350;
                else
                    vare0 = -350;
                vard4 <<= 2;
                if (pPlayer->moto_do_bump)
                {
                    pPlayer->vel.x += (vard4>>6)*(sintable[(vardc*-51+vard8+512)&2047]<<4);
                    pPlayer->vel.y += (vard4>>6)*(sintable[(vardc*-51+vard8)&2047]<<4);
                    pPlayer->q16ang = F16((vard8-(vare0>>5))&2047);
                }
                else
                {
                    pPlayer->vel.x += (vard4>>7)*(sintable[(vardc*-51+vard8+512)&2047]<<4);
                    pPlayer->vel.y += (vard4>>7)*(sintable[(vardc*-51+vard8)&2047]<<4);
                    pPlayer->q16ang = F16((vard8-(vare0>>6))&2047);
                }
            }
            if (pPlayer->not_on_water)
                if (pPlayer->MotoSpeed > 50)
                    pPlayer->MotoSpeed -= (pPlayer->MotoSpeed>>1);
        }
    }

    if (pPlayer->cursectnum == -1)
    {
        if (pSprite->extra > 0 && ud.clipping == 0)
        {
            quickkill(pPlayer);
            A_PlaySound(SQUISHED, pPlayer->i);
        }

        pPlayer->cursectnum = 0;
    }

    int sectorLotag       = sector[pPlayer->cursectnum].lotag;

    if (RR)
    {
        if (sectorLotag == 867)
        {
            int spriteNum = headspritesect[pPlayer->cursectnum];
            while (spriteNum >= 0)
            {
                int const nextSprite = nextspritesect[spriteNum];
                if (sprite[spriteNum].picnum == TILE_RRTILE380)
                    if (sprite[spriteNum].z - ZOFFSET3 < pPlayer->pos.z)
                        sectorLotag = 2;
                spriteNum = nextSprite;
            }
        }
        else if (sectorLotag == 7777)
            if (ud.volume_number == 1 && ud.level_number == 6)
                g_lastLevel = 1;

        if (sectorLotag == 848 && sector[pPlayer->cursectnum].floorpicnum == TILE_WATERTILE2)
            sectorLotag = 1;

        if (sectorLotag == 857)
            pSprite->clipdist = 1;
        else
            pSprite->clipdist = 64;
    }

    pPlayer->spritebridge = 0;
    //pPlayer->sbs          = 0;

    int32_t floorZ, ceilZ, highZhit, lowZhit;
    if (!RR || pSprite->clipdist == 64)
        getzrange((vec3_t *)pPlayer, pPlayer->cursectnum, &ceilZ, &highZhit, &floorZ, &lowZhit, 163, CLIPMASK0);
    else
        getzrange((vec3_t *)pPlayer, pPlayer->cursectnum, &ceilZ, &highZhit, &floorZ, &lowZhit, 4, CLIPMASK0);

#ifdef YAX_ENABLE
    getzsofslope_player(pPlayer->cursectnum, pPlayer->pos.x, pPlayer->pos.y, &pPlayer->truecz, &pPlayer->truefz);
#else
    getzsofslope(pPlayer->cursectnum, pPlayer->pos.x, pPlayer->pos.y, &pPlayer->truecz, &pPlayer->truefz);
#endif
    int const trueFloorZ    = pPlayer->truefz;
    int const trueFloorDist = klabs(pPlayer->pos.z - trueFloorZ);

    if ((lowZhit & 49152) == 16384 && sectorLotag == 1 && trueFloorDist > PHEIGHT + ZOFFSET2)
        sectorLotag = 0;

    actor[pPlayer->i].floorz   = floorZ;
    actor[pPlayer->i].ceilingz = ceilZ;

    if (highZhit >= 0 && (highZhit&49152) == 49152)
    {
        highZhit &= (MAXSPRITES-1);

        if (sprite[highZhit].statnum == STAT_ACTOR && sprite[highZhit].extra >= 0)
        {
            highZhit = 0;
            ceilZ    = pPlayer->truecz;
        }
        if (RR)
        {
            if (sprite[highZhit].picnum == TILE_RRTILE3587)
            {
                if (!pPlayer->stairs)
                {
                    pPlayer->stairs = 10;
                    if (TEST_SYNC_KEY(playerBits, SK_JUMP) && (!RRRA || !pPlayer->OnMotorcycle))
                    {
                        highZhit = 0;
                        ceilZ = pPlayer->truecz;
                    }
                }
                else
                    pPlayer->stairs--;
            }
        }
    }

    if (lowZhit >= 0 && (lowZhit&49152) == 49152)
    {
        int spriteNum = lowZhit&(MAXSPRITES-1);

        if ((sprite[spriteNum].cstat&33) == 33)
        {
            sectorLotag             = 0;
            pPlayer->footprintcount = 0;
            pPlayer->spritebridge   = 1;
            //pPlayer->sbs            = spriteNum;
        }
        else if (!RRRA)
            goto check_enemy_sprite;

        if (RRRA)
        {
            if (pPlayer->OnMotorcycle)
            {
                if (A_CheckEnemySprite(&sprite[spriteNum]))
                {
                    actor[spriteNum].picnum = TILE_MOTOHIT;
                    actor[spriteNum].extra = 2+(pPlayer->MotoSpeed>>1);
                    pPlayer->MotoSpeed -= pPlayer->MotoSpeed >> 4;
                }
            }
            if (pPlayer->OnBoat)
            {
                if (A_CheckEnemySprite(&sprite[spriteNum]))
                {
                    actor[spriteNum].picnum = TILE_MOTOHIT;
                    actor[spriteNum].extra = 2+(pPlayer->MotoSpeed>>1);
                    pPlayer->MotoSpeed -= pPlayer->MotoSpeed >> 4;
                }
            }
            else
            {
check_enemy_sprite:
                if (A_CheckEnemySprite(&sprite[spriteNum]) && sprite[spriteNum].xrepeat > 24
                     && klabs(pSprite->z - sprite[spriteNum].z) < (84 << 8))
                {
                    // TX: I think this is what makes the player slide off enemies... might
                    // be a good sprite flag to add later.
                    // Helix: there's also SLIDE_ABOVE_ENEMY.
                    int spriteAng = getangle(sprite[spriteNum].x - pPlayer->pos.x,
                                               sprite[spriteNum].y - pPlayer->pos.y);
                    pPlayer->vel.x -= sintable[(spriteAng + 512) & 2047] << 4;
                    pPlayer->vel.y -= sintable[spriteAng & 2047] << 4;
                }
            }
        }
        if (RR)
        {
            if (sprite[spriteNum].picnum == TILE_RRTILE3587)
            {
                if (!pPlayer->stairs)
                {
                    pPlayer->stairs = 10;
                    if (TEST_SYNC_KEY(playerBits, SK_CROUCH) && (!RRRA || !pPlayer->OnMotorcycle))
                    {
                        ceilZ = sprite[spriteNum].z;
                        highZhit = 0;
                        floorZ = sprite[spriteNum].z + ZOFFSET6;
                    }
                }
                else
                    pPlayer->stairs--;
            }
            else if (sprite[spriteNum].picnum == TILE_TOILET || sprite[spriteNum].picnum == TILE_RRTILE2121)
            {
                if (TEST_SYNC_KEY(playerBits, SK_CROUCH) && (!RRRA || !pPlayer->OnMotorcycle))
                {
                    A_PlaySound(436, pPlayer->i);
                    pPlayer->last_pissed_time = 4000;
                    pPlayer->eat = 0;
                }
            }
        }
    }

    if (pSprite->extra > 0)
        P_IncurDamage(pPlayer);
    else
    {
        pSprite->extra                  = 0;
        pPlayer->inv_amount[GET_SHIELD] = 0;
    }

    pPlayer->last_extra = pSprite->extra;
    pPlayer->loogcnt    = (pPlayer->loogcnt > 0) ? pPlayer->loogcnt - 1 : 0;

    if (pPlayer->fist_incs && P_DoFist(pPlayer) && !RR) return;

    if (pPlayer->timebeforeexit > 1 && pPlayer->last_extra > 0)
    {
        if (--pPlayer->timebeforeexit == GAMETICSPERSEC*5)
        {
            FX_StopAllSounds();
            S_ClearSoundLocks();

            if (pPlayer->customexitsound >= 0)
            {
                S_PlaySound(pPlayer->customexitsound);
                P_DoQuote(QUOTE_WEREGONNAFRYYOURASS,pPlayer);
            }
        }
        else if (pPlayer->timebeforeexit == 1)
        {
            for (bssize_t TRAVERSE_CONNECT(playerNum))
                g_player[playerNum].ps->gm = MODE_EOL;

            if (RR && ud.level_number == 6 && ud.volume_number == 0)
                g_turdLevel = 1;
            if (!RR && ud.from_bonus)
            {
                ud.level_number   = ud.from_bonus;
                m_level_number = ud.level_number;
                ud.from_bonus     = 0;
            }
            else
            {
                ud.level_number   = (++ud.level_number < MAXLEVELS) ? ud.level_number : 0;
                m_level_number = ud.level_number;
            }
            return;
        }
    }

    if (pPlayer->pals.f > 0)
        P_HandlePal(pPlayer);

    if (pPlayer->fta > 0 && --pPlayer->fta == 0)
    {
        pub = pus = NUMPAGES;
        pPlayer->ftq = 0;
    }

    if (g_levelTextTime > 0)
        g_levelTextTime--;

    if (pSprite->extra <= 0)
    {
        P_Dead(playerNum, sectorLotag, floorZ, ceilZ);
        return;
    }

    if (pPlayer->transporter_hold > 0)
    {
        pPlayer->transporter_hold--;
        if (pPlayer->transporter_hold == 0 && pPlayer->on_warping_sector)
            pPlayer->transporter_hold = 2;
    }
    else if (pPlayer->transporter_hold < 0)
        pPlayer->transporter_hold++;

    if (pPlayer->newowner >= 0)
    {
        P_UpdatePosWhenViewingCam(pPlayer);
        fi.doincrements(&ps[playerNum]);

        if ((WW2GI ? PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) : pPlayer->curr_weapon) == HANDREMOTE_WEAPON)
            P_ProcessWeapon(playerNum);

        return;
    }

    pPlayer->rotscrnang -= (pPlayer->rotscrnang >> 1);

    if (pPlayer->rotscrnang && !(pPlayer->rotscrnang >> 1))
        pPlayer->rotscrnang -= ksgn(pPlayer->rotscrnang);

    pPlayer->look_ang -= (pPlayer->look_ang >> 2);

    if (pPlayer->look_ang && !(pPlayer->look_ang >> 2))
        pPlayer->look_ang -= ksgn(pPlayer->look_ang);

    if (TEST_SYNC_KEY(playerBits, SK_LOOK_LEFT) && (!RRRA || !pPlayer->OnMotorcycle))
    {
        // look_left
        if (VM_OnEvent(EVENT_LOOKLEFT,pPlayer->i,playerNum) == 0)
        {
            pPlayer->look_ang -= 152;
            pPlayer->rotscrnang += 24;
        }
    }

    if (TEST_SYNC_KEY(playerBits, SK_LOOK_RIGHT) && (!RRRA || !pPlayer->OnMotorcycle))
    {
        // look_right
        if (VM_OnEvent(EVENT_LOOKRIGHT,pPlayer->i,playerNum) == 0)
        {
            pPlayer->look_ang += 152;
            pPlayer->rotscrnang -= 24;
        }
    }

    if (RRRA && pPlayer->sea_sick)
    {
        if (pPlayer->sea_sick < 250)
        {
            if (pPlayer->sea_sick >= 180)
                pPlayer->rotscrnang += 24;
            else if (pPlayer->sea_sick >= 130)
                pPlayer->rotscrnang -= 24;
            else if (pPlayer->sea_sick >= 70)
                pPlayer->rotscrnang += 24;
            else if (pPlayer->sea_sick >= 20)
                pPlayer->rotscrnang += 24;
        }
        if (pPlayer->sea_sick < 250)
            pPlayer->look_ang += (krand2()&255)-128;
    }

    int                  velocityModifier = TICSPERFRAME;
    const uint8_t *const weaponFrame      = &pPlayer->kickback_pic;
    int                  floorZOffset     = 40;
    int const            playerShrunk     = (pSprite->yrepeat < (RR ? 8 : 32));

    if (pPlayer->on_crane >= 0)
        goto HORIZONLY;

    pPlayer->weapon_sway = (pSprite->xvel < 32 || pPlayer->on_ground == 0 || pPlayer->bobcounter == 1024)
                           ? (((pPlayer->weapon_sway & 2047) > (1024 + 96))
                           ? (pPlayer->weapon_sway - 96)
                           : (((pPlayer->weapon_sway & 2047) < (1024 - 96)))
                           ? (pPlayer->weapon_sway + 96)
                           : 1024)
                           : pPlayer->bobcounter;

    // NOTE: This silently wraps if the difference is too great, e.g. used to do
    // that when teleported by silent SE7s.
    pSprite->xvel = ksqrt(uhypsq(pPlayer->pos.x - pPlayer->bobpos.x, pPlayer->pos.y - pPlayer->bobpos.y));

    if (pPlayer->on_ground)
        pPlayer->bobcounter += sprite[pPlayer->i].xvel>>1;

    if (ud.clipping == 0 && ((uint16_t)pPlayer->cursectnum >= MAXSECTORS || sector[pPlayer->cursectnum].floorpicnum == TILE_MIRROR))
    {
        pPlayer->pos.x = pPlayer->opos.x;
        pPlayer->pos.y = pPlayer->opos.y;
    }
    else
    {
        pPlayer->opos.x = pPlayer->pos.x;
        pPlayer->opos.y = pPlayer->pos.y;
    }

    pPlayer->bobpos.x = pPlayer->pos.x;
    pPlayer->bobpos.y = pPlayer->pos.y;
    pPlayer->opos.z   = pPlayer->pos.z;
    pPlayer->opyoff   = pPlayer->pyoff;

    if (pPlayer->one_eighty_count < 0)
    {
        pPlayer->one_eighty_count += 128;
        pPlayer->q16ang += F16(128);
    }

    // Shrinking code

    if (RR)
    {
        if (sectorLotag == 17 || (RRRA && sectorLotag == 18))
        {
            if (getanimationgoal(&sector[pPlayer->cursectnum].floorz) >= 0)
            {
                if (!S_CheckSoundPlaying(pPlayer->i, 432))
                    A_PlaySound(432, pPlayer->i);
            }
            else
                S_StopSound(432);
        }
        if (pPlayer->sea_sick_stat)
        {
            pPlayer->pycount += 32;
            pPlayer->pycount &= 2047;
            if (pPlayer->sea_sick)
                pPlayer->pyoff = sintable[pPlayer->pycount]>>2;
            else
                pPlayer->pyoff = sintable[pPlayer->pycount]>>7;
        }
    }

    if (sectorLotag == ST_2_UNDERWATER)
        P_DoWater(playerNum, playerBits, floorZ, ceilZ);
    else if (!RR && pPlayer->jetpack_on)
        P_DoJetpack(playerNum, playerBits, playerShrunk, sectorLotag, floorZ);
    else
    {
        pPlayer->airleft  = 15 * GAMETICSPERSEC;  // 13 seconds
        pPlayer->scuba_on = 0;

        if (sectorLotag == ST_1_ABOVE_WATER && pPlayer->spritebridge == 0)
        {
            floorZOffset = 12;

            if (playerShrunk == 0)
            {
                floorZOffset      = 34;
                pPlayer->pycount += 32;
                pPlayer->pycount &= 2047;
                pPlayer->pyoff    = sintable[pPlayer->pycount] >> 6;
            }

            if (playerShrunk == 0 && trueFloorDist <= PHEIGHT)
            {
                if (pPlayer->on_ground == 1)
                {
                    if (pPlayer->dummyplayersprite < 0)
                        pPlayer->dummyplayersprite = fi.spawn(pPlayer->i,TILE_PLAYERONWATER);

                    pPlayer->footprintcount = 6;
                    //sprite[pPlayer->dummyplayersprite].cstat |= 32768;
                    //sprite[pPlayer->dummyplayersprite].pal = sprite[pPlayer->i].pal;
                    pPlayer->footprintpal                  = 0;
                    pPlayer->footprintshade                = 0;
                    if (sector[pPlayer->cursectnum].floorpicnum == TILE_FLOORSLIME)
                    {
                        pPlayer->footprintpal = 8;
                        pPlayer->footprintshade = 0;
                    }
                    else if (RRRA && (sector[pPlayer->cursectnum].floorpicnum == TILE_RRTILE7756 || sector[pPlayer->cursectnum].floorpicnum == TILE_RRTILE7888))
                    {
                        pPlayer->footprintpal = 0;
                        pPlayer->footprintshade = 40;
                    }
                }
            }
        }
        else if ((!RRRA || pPlayer->OnMotorcycle) && pPlayer->footprintcount > 0 && pPlayer->on_ground)
        {
            if (pPlayer->cursectnum >= 0 && (sector[pPlayer->cursectnum].floorstat & 2) != 2)
            {
                int spriteNum = -1;

                for (spriteNum = headspritesect[pPlayer->cursectnum]; spriteNum >= 0; spriteNum = nextspritesect[spriteNum])
                {
                    if (sprite[spriteNum].picnum == TILE_FOOTPRINTS || sprite[spriteNum].picnum == TILE_FOOTPRINTS2 ||
                        sprite[spriteNum].picnum == TILE_FOOTPRINTS3 || sprite[spriteNum].picnum == TILE_FOOTPRINTS4)
                    {
                        if (klabs(sprite[spriteNum].x - pPlayer->pos.x) < 384 &&
                            klabs(sprite[spriteNum].y - pPlayer->pos.y) < 384)
                            break;
                    }
                }

                if (spriteNum < 0)
                {
                    pPlayer->footprintcount--;
                    if (pPlayer->cursectnum >= 0 && sector[pPlayer->cursectnum].lotag == 0 &&
                        sector[pPlayer->cursectnum].hitag == 0)
#ifdef YAX_ENABLE
                        if (yax_getbunch(pPlayer->cursectnum, YAX_FLOOR) < 0 || (sector[pPlayer->cursectnum].floorstat & 512))
#endif
                        {
                            switch (krand2() & 3)
                            {
                                case 0: spriteNum  = fi.spawn(pPlayer->i, TILE_FOOTPRINTS); break;
                                case 1: spriteNum  = fi.spawn(pPlayer->i, TILE_FOOTPRINTS2); break;
                                case 2: spriteNum  = fi.spawn(pPlayer->i, TILE_FOOTPRINTS3); break;
                                default: spriteNum = fi.spawn(pPlayer->i, TILE_FOOTPRINTS4); break;
                            }
                            sprite[spriteNum].pal   = pPlayer->footprintpal;
                            sprite[spriteNum].shade = pPlayer->footprintshade;
                        }
                }
            }
        }

        if (pPlayer->pos.z < (floorZ-(floorZOffset<<8)))  //falling
        {
            // not jumping or crouching

            if ((!TEST_SYNC_KEY(playerBits, SK_JUMP) && !TEST_SYNC_KEY(playerBits, SK_CROUCH)) && pPlayer->on_ground &&
                (sector[pPlayer->cursectnum].floorstat & 2) && pPlayer->pos.z >= (floorZ - (floorZOffset << 8) - ZOFFSET2))
                pPlayer->pos.z = floorZ - (floorZOffset << 8);
            else
            {
                if (RRRA && (pPlayer->OnMotorcycle || pPlayer->OnBoat) && floorZ - (floorZOffset << 9) > pPlayer->pos.z)
                {
                    if (pPlayer->moto_on_ground)
                    {
                        pPlayer->moto_bump_target = 80;
                        pPlayer->moto_bump_fast = 1;
                        pPlayer->vel.z -= g_spriteGravity*(pPlayer->MotoSpeed>>4);
                        pPlayer->moto_on_ground = 0;
                        if (A_CheckSoundPlaying(pPlayer->i, 188))
                            S_StopEnvSound(188, pPlayer->i);
                        A_PlaySound(189, pPlayer->i);
                    }
                    else
                    {
                        pPlayer->vel.z += g_spriteGravity-80+(120-pPlayer->MotoSpeed);
                        if (!A_CheckSoundPlaying(pPlayer->i, 189) && !A_CheckSoundPlaying(pPlayer->i, 190))
                            A_PlaySound(190, pPlayer->i);
                    }
                }
                else
                    pPlayer->vel.z    += (g_spriteGravity + 80);  // (TICSPERFRAME<<6);

                if (pPlayer->vel.z >= (4096 + 2048))
                    pPlayer->vel.z = (4096 + 2048);

                if (pPlayer->vel.z > 2400 && pPlayer->falling_counter < 255)
                {
                    pPlayer->falling_counter++;
                    if (pPlayer->falling_counter >= 38 && !A_CheckSoundPlaying(pPlayer->i, -1, CHAN_VOICE))
                    {
                        A_PlaySound(DUKE_SCREAM, pPlayer->i, CHAN_VOICE);
                    }
                }

                if ((pPlayer->pos.z + pPlayer->vel.z) >= (floorZ - (floorZOffset << 8)) && pPlayer->cursectnum >= 0)  // hit the ground
                {
                    if (sector[pPlayer->cursectnum].lotag != ST_1_ABOVE_WATER)
                    {
                        if (RRRA)
                            pPlayer->moto_on_ground = 1;
                        if (pPlayer->falling_counter > 62 || (RRRA && pPlayer->falling_counter > 2 && sector[pPlayer->cursectnum].lotag == 802))
                            quickkill(pPlayer);
                        else if (pPlayer->falling_counter > 9)
                        {
                            // Falling damage.
                            pSprite->extra -= pPlayer->falling_counter - (krand2() & 3);

                            if (pSprite->extra <= 0)
                                A_PlaySound(SQUISHED, pPlayer->i);
                            else
                            {
                                A_PlaySound(DUKE_LAND, pPlayer->i);
                                A_PlaySound(DUKE_LAND_HURT, pPlayer->i);
                            }

                            P_PalFrom(pPlayer, 32, 16, 0, 0);
                        }
                        else if (pPlayer->vel.z > 2048)
                        {
                            if (RRRA && pPlayer->OnMotorcycle)
                            {
                                if (A_CheckSoundPlaying(pPlayer->i, 190))
                                    S_StopEnvSound(pPlayer->i, 190);
                                A_PlaySound(191, pPlayer->i);
                                pPlayer->moto_turb = 12;
                            }
                            else
                                A_PlaySound(DUKE_LAND, pPlayer->i);
                        }
                        else if (RRRA && pPlayer->vel.z > 1024 && pPlayer->OnMotorcycle)
                        {
                            A_PlaySound(DUKE_LAND, pPlayer->i);
                            pPlayer->moto_turb = 12;
                        }
                    }
                    pPlayer->on_ground = 1;
                }
                else
                    pPlayer->on_ground = 0;
            }
        }
        else
        {
            pPlayer->falling_counter = 0;

            S_StopEnvSound(-1, pPlayer->i, CHAN_VOICE);

            if ((sectorLotag != ST_1_ABOVE_WATER && sectorLotag != ST_2_UNDERWATER) &&
                (pPlayer->on_ground == 0 && pPlayer->vel.z > (6144 >> 1)))
                pPlayer->hard_landing = pPlayer->vel.z>>10;

            pPlayer->on_ground = 1;

            if (floorZOffset==40)
            {
                //Smooth on the ground
                int Zdiff = ((floorZ - (floorZOffset << 8)) - pPlayer->pos.z) >> 1;

                if (klabs(Zdiff) < 256)
                    Zdiff = 0;

                pPlayer->pos.z += ((klabs(Zdiff) >= 256) ? (((floorZ - (floorZOffset << 8)) - pPlayer->pos.z) >> 1) : 0);
                pPlayer->vel.z -= 768;

                if (pPlayer->vel.z < 0)
                    pPlayer->vel.z = 0;
            }
            else if (pPlayer->jumping_counter == 0)
            {
                pPlayer->pos.z += ((floorZ - (floorZOffset << 7)) - pPlayer->pos.z) >> 1;  // Smooth on the water

                if (pPlayer->on_warping_sector == 0 && pPlayer->pos.z > floorZ - ZOFFSET2)
                {
                    pPlayer->pos.z = floorZ - ZOFFSET2;
                    pPlayer->vel.z >>= 1;
                }
            }

            pPlayer->on_warping_sector = 0;

            if (TEST_SYNC_KEY(playerBits, SK_CROUCH) && (!RRRA || !pPlayer->OnMotorcycle))
            {
                // crouching
                if (VM_OnEvent(EVENT_CROUCH,pPlayer->i,playerNum) == 0)
                {
                    pPlayer->pos.z += (2048+768);
                    pPlayer->crack_time = 777;
                }
            }

            // jumping
            if (!TEST_SYNC_KEY(playerBits, SK_JUMP) && (!RRRA || !pPlayer->OnMotorcycle) && pPlayer->jumping_toggle == 1)
                pPlayer->jumping_toggle = 0;
            else if (TEST_SYNC_KEY(playerBits, SK_JUMP) && (!RRRA || !pPlayer->OnMotorcycle) && pPlayer->jumping_toggle == 0)
            {
                if (pPlayer->jumping_counter == 0)
                    if ((floorZ-ceilZ) > (56<<8))
                    {
                        if (VM_OnEvent(EVENT_JUMP,pPlayer->i,playerNum) == 0)
                        {
                            pPlayer->jumping_counter = 1;
                            pPlayer->jumping_toggle = 1;
                        }
                    }
            }

            if (!RR && pPlayer->jumping_counter && !TEST_SYNC_KEY(playerBits, SK_JUMP))
                pPlayer->jumping_toggle = 0;
        }

        if (pPlayer->jumping_counter)
        {
            if (!TEST_SYNC_KEY(playerBits, SK_JUMP) && (!RRRA || !pPlayer->OnMotorcycle) && pPlayer->jumping_toggle == 1)
                pPlayer->jumping_toggle = 0;

            if (pPlayer->jumping_counter < (RR ? 768 : (1024+256)))
            {
                if (sectorLotag == ST_1_ABOVE_WATER && pPlayer->jumping_counter > 768)
                {
                    pPlayer->jumping_counter = 0;
                    pPlayer->vel.z = -512;
                }
                else
                {
                    pPlayer->vel.z -= (sintable[(2048-128+pPlayer->jumping_counter)&2047])/12;
                    pPlayer->jumping_counter += 180;
                    pPlayer->on_ground = 0;
                }
            }
            else
            {
                pPlayer->jumping_counter = 0;
                pPlayer->vel.z = 0;
            }
        }

        pPlayer->pos.z += pPlayer->vel.z;

        if (pPlayer->pos.z < (ceilZ+ZOFFSET6))
        {
            pPlayer->jumping_counter = 0;
            if (pPlayer->vel.z < 0)
                pPlayer->vel.x = pPlayer->vel.y = 0;
            pPlayer->vel.z = 128;
            pPlayer->pos.z = ceilZ+ZOFFSET6;
        }
    }

    if (P_CheckLockedMovement(playerNum) & IL_NOMOVE)
    {
        velocityModifier = 0;
        pPlayer->vel.x   = 0;
        pPlayer->vel.y   = 0;
    }
    else if (thisPlayer.input->q16avel)
        pPlayer->crack_time = 777;

    if (pPlayer->spritebridge == 0)
    {
        int const floorPicnum = sector[pSprite->sectnum].floorpicnum;

        if (!RR && (floorPicnum == TILE_PURPLELAVA || sector[pSprite->sectnum].ceilingpicnum == TILE_PURPLELAVA))
        {
            if (pPlayer->inv_amount[GET_BOOTS] > 0)
            {
                pPlayer->inv_amount[GET_BOOTS]--;
                pPlayer->inven_icon = ICON_BOOTS;
                if (pPlayer->inv_amount[GET_BOOTS] <= 0)
                    P_SelectNextInvItem(pPlayer);
            }
            else
            {
                if (!A_CheckSoundPlaying(pPlayer->i,DUKE_LONGTERM_PAIN))
                    A_PlaySound(DUKE_LONGTERM_PAIN,pPlayer->i);
                P_PalFrom(pPlayer, 32, 0, 8, 0);
                pSprite->extra--;
            }
        }

        if (RRRA && pPlayer->on_ground && trueFloorDist <= PHEIGHT+ZOFFSET2 && (floorPicnum == TILE_RRTILE7768 || floorPicnum == TILE_RRTILE7820))
        {
            if ((krand2() & 3) == 1)
            {
                if (pPlayer->OnMotorcycle)
                    pSprite->extra -= 2;
                else
                    pSprite->extra -= 4;
                A_PlaySound(DUKE_LONGTERM_PAIN, pPlayer->i);
            }
        }
        else if (pPlayer->on_ground && trueFloorDist <= PHEIGHT+ZOFFSET2 && P_CheckFloorDamage(pPlayer, floorPicnum))
        {
            P_DoQuote(QUOTE_BOOTS_ON, pPlayer);
            pPlayer->inv_amount[GET_BOOTS] -= 2;
            if (pPlayer->inv_amount[GET_BOOTS] <= 0)
            {
                pPlayer->inv_amount[GET_BOOTS] = 0;
                P_SelectNextInvItem(pPlayer);
            }
        }
    }

    if (pPlayer->vel.x || pPlayer->vel.y || thisPlayer.input->fvel || thisPlayer.input->svel)
    {
        pPlayer->crack_time = 777;

        int const checkWalkSound = sintable[pPlayer->bobcounter & 2047] >> 12;

        if (RRRA)
        {
            if (pPlayer->spritebridge == 0 && pPlayer->on_ground)
            {
                if (sectorLotag == ST_1_ABOVE_WATER)
                    pPlayer->not_on_water = 0;
                else if (pPlayer->OnBoat)
                {
                    if (sectorLotag == 1234)
                        pPlayer->not_on_water = 0;
                    else
                        pPlayer->not_on_water = 1;
                }
                else
                    pPlayer->not_on_water = 1;
            }
        }

        if ((trueFloorDist < PHEIGHT + ZOFFSET3))
        {
            if (checkWalkSound == 1 || checkWalkSound == 3)
            {
                if (pPlayer->spritebridge == 0 && pPlayer->walking_snd_toggle == 0 && pPlayer->on_ground)
                {
                    switch (sectorLotag)
                    {
                        case 0:
                        {
                            int const walkPicnum = (lowZhit >= 0 && (lowZhit & 49152) == 49152)
                                                   ? TrackerCast(sprite[lowZhit & (MAXSPRITES - 1)].picnum)
                                                   : TrackerCast(sector[pPlayer->cursectnum].floorpicnum);

                            if (!RR)
                                switch (DYNAMICTILEMAP(walkPicnum))
                                {
                                    case PANNEL1__STATIC:
                                    case PANNEL2__STATIC:
                                        A_PlaySound(DUKE_WALKINDUCTS, pPlayer->i);
                                        pPlayer->walking_snd_toggle = 1;
                                        break;
                                }
                        }
                        break;

                        case ST_1_ABOVE_WATER:
                            if ((krand2() & 1) == 0 && (!RRRA || (!pPlayer->OnBoat && !pPlayer->OnMotorcycle && sector[pPlayer->cursectnum].lotag != 321)))
                                A_PlaySound(DUKE_ONWATER, pPlayer->i);
                            pPlayer->walking_snd_toggle = 1;
                            break;
                    }
                }
            }
            else if (pPlayer->walking_snd_toggle > 0)
                pPlayer->walking_snd_toggle--;
        }

        if (pPlayer->jetpack_on == 0 && pPlayer->inv_amount[GET_STEROIDS] > 0 && pPlayer->inv_amount[GET_STEROIDS] < 400)
            velocityModifier <<= 1;

        pPlayer->vel.x += (((thisPlayer.input->fvel) * velocityModifier) << 6);
        pPlayer->vel.y += (((thisPlayer.input->svel) * velocityModifier) << 6);

        int playerSpeedReduction = 0;
        
        if (!RRRA && pPlayer->on_ground && (TEST_SYNC_KEY(playerBits, SK_CROUCH)
                  || (*weaponFrame > 10 && pPlayer->curr_weapon == KNEE_WEAPON)))
            playerSpeedReduction = 0x2000;
        else if (sectorLotag == ST_2_UNDERWATER)
            playerSpeedReduction = 0x1400;

        pPlayer->vel.x = mulscale16(pPlayer->vel.x, pPlayer->runspeed - playerSpeedReduction);
        pPlayer->vel.y = mulscale16(pPlayer->vel.y, pPlayer->runspeed - playerSpeedReduction);

        if (RR)
        {
            if (RRRA)
            {
                if (sector[pPlayer->cursectnum].floorpicnum == TILE_RRTILE7888)
                {
                    if (pPlayer->OnMotorcycle && pPlayer->on_ground)
                        pPlayer->moto_on_oil = 1;
                }
                else if (sector[pPlayer->cursectnum].floorpicnum == TILE_RRTILE7889)
                {
                    if (pPlayer->OnMotorcycle)
                    {
                        if (pPlayer->on_ground)
                            pPlayer->moto_on_mud = 1;
                    }
                    else if (pPlayer->inv_amount[GET_BOOTS] > 0)
                        pPlayer->inv_amount[GET_BOOTS]--;
                    else
                    {
                        pPlayer->vel.x = mulscale16(pPlayer->vel.x, pPlayer->runspeed);
                        pPlayer->vel.y = mulscale16(pPlayer->vel.y, pPlayer->runspeed);
                    }
                }
            }
            if (sector[pPlayer->cursectnum].floorpicnum == TILE_RRTILE3073 || sector[pPlayer->cursectnum].floorpicnum == TILE_RRTILE2702)
            {
                if (RRRA && pPlayer->OnMotorcycle)
                {
                    if (pPlayer->on_ground)
                    {
                        pPlayer->vel.x = mulscale16(pPlayer->vel.x, pPlayer->runspeed-0x1800);
                        pPlayer->vel.y = mulscale16(pPlayer->vel.y, pPlayer->runspeed-0x1800);
                    }
                }
                else if (pPlayer->inv_amount[GET_BOOTS] > 0)
                    pPlayer->inv_amount[GET_BOOTS]--;
                else
                {
                    pPlayer->vel.x = mulscale16(pPlayer->vel.x, pPlayer->runspeed-0x1800);
                    pPlayer->vel.y = mulscale16(pPlayer->vel.y, pPlayer->runspeed-0x1800);
                }
            }
        }

        if (klabs(pPlayer->vel.x) < 2048 && klabs(pPlayer->vel.y) < 2048)
            pPlayer->vel.x = pPlayer->vel.y = 0;

        if (playerShrunk)
        {
            pPlayer->vel.x = mulscale16(pPlayer->vel.x, pPlayer->runspeed - (pPlayer->runspeed >> 1) + (pPlayer->runspeed >> 2));
            pPlayer->vel.y = mulscale16(pPlayer->vel.y, pPlayer->runspeed - (pPlayer->runspeed >> 1) + (pPlayer->runspeed >> 2));
        }
    }

HORIZONLY:;
    int stepHeight = (sectorLotag == ST_1_ABOVE_WATER || pPlayer->spritebridge == 1) ? pPlayer->autostep_sbw : pPlayer->autostep;

#ifdef EDUKE32_TOUCH_DEVICES
    if (TEST_SYNC_KEY(playerBits, SK_CROUCH))
        stepHeight = pPlayer->autostep_sbw;
#endif

    if (ud.clipping)
    {
        pPlayer->pos.x += pPlayer->vel.x >> 14;
        pPlayer->pos.y += pPlayer->vel.y >> 14;
        updatesector(pPlayer->pos.x, pPlayer->pos.y, &pPlayer->cursectnum);
        changespritesect(pPlayer->i, pPlayer->cursectnum);
        // This makes the player view lower when shrunk.  NOTE that it can get the
        // view below the sector floor (and does, when on the ground).
        if (pPlayer->jetpack_on == 0 && sectorLotag != ST_2_UNDERWATER && sectorLotag != ST_1_ABOVE_WATER && playerShrunk)
            pPlayer->pos.z += ZOFFSET5;
        if (RRRA && pPlayer->hurt_delay2 > 0)
            pPlayer->hurt_delay2--;
    }
    else
    {
#ifdef YAX_ENABLE
        int const playerSectNum = pPlayer->cursectnum;
        int16_t   ceilingBunch, floorBunch;

        if (playerSectNum >= 0)
            yax_getbunches(playerSectNum, &ceilingBunch, &floorBunch);

        // This updatesectorz conflicts with Duke3D's way of teleporting through water,
        // so make it a bit conditional... OTOH, this way we have an ugly z jump when
        // changing from above water to underwater

        if ((playerSectNum >= 0 && !(sector[playerSectNum].lotag == ST_1_ABOVE_WATER && pPlayer->on_ground && floorBunch >= 0))
            && ((floorBunch >= 0 && !(sector[playerSectNum].floorstat & 512))
                || (ceilingBunch >= 0 && !(sector[playerSectNum].ceilingstat & 512))))
        {
            pPlayer->cursectnum += MAXSECTORS;  // skip initial z check, restored by updatesectorz
            updatesectorz(pPlayer->pos.x, pPlayer->pos.y, pPlayer->pos.z, &pPlayer->cursectnum);
        }
#endif
        int spriteNum = clipmove((vec3_t *)pPlayer, &pPlayer->cursectnum, pPlayer->vel.x, pPlayer->vel.y, 164,
                                 (4L << 8), stepHeight, CLIPMASK0);

        // This makes the player view lower when shrunk.  NOTE that it can get the
        // view below the sector floor (and does, when on the ground).
        if (pPlayer->jetpack_on == 0 && sectorLotag != ST_2_UNDERWATER && sectorLotag != ST_1_ABOVE_WATER && playerShrunk)
            pPlayer->pos.z += ZOFFSET5;

        if (spriteNum)
            P_CheckTouchDamage(pPlayer, spriteNum);
        else if(RRRA && pPlayer->hurt_delay2 > 0)
            pPlayer->hurt_delay2--;

        if (RR)
        {
            if ((spriteNum & 49152) == 32768)
            {
                int const wallNum = spriteNum&(MAXWALLS-1);
                if (RRRA && pPlayer->OnMotorcycle)
                {
                    int16_t var104, var108, var10c;
                    var104 = 0;
                    var108 = getangle(wall[wall[wallNum].point2].x-wall[wallNum].x,wall[wall[wallNum].point2].y-wall[wallNum].y);
                    var10c = klabs(fix16_to_int(pPlayer->q16ang)-var108);
                    switch (krand2()&1)
                    {
                        case 0:
                            pPlayer->q16ang += F16(pPlayer->MotoSpeed>>1);
                            break;
                        case 1:
                            pPlayer->q16ang -= F16(pPlayer->MotoSpeed>>1);
                            break;
                    }
                    if (var10c >= 441 && var10c <= 581)
                    {
                        var104 = (pPlayer->MotoSpeed*pPlayer->MotoSpeed)>>8;
                        pPlayer->MotoSpeed = 0;
                        if (A_CheckSoundPlaying(pPlayer->i, 238) == 0)
                            A_PlaySound(238,pPlayer->i);
                    }
                    else if (var10c >= 311 && var10c <= 711)
                    {
                        var104 = (pPlayer->MotoSpeed*pPlayer->MotoSpeed)>>11;
                        pPlayer->MotoSpeed -= (pPlayer->MotoSpeed>>1)+(pPlayer->MotoSpeed>>2);
                        if (A_CheckSoundPlaying(pPlayer->i, 238) == 0)
                            A_PlaySound(238,pPlayer->i);
                    }
                    else if (var10c >= 111 && var10c <= 911)
                    {
                        var104 = (pPlayer->MotoSpeed*pPlayer->MotoSpeed)>>14;
                        pPlayer->MotoSpeed -= (pPlayer->MotoSpeed>>1);
                        if (A_CheckSoundPlaying(pPlayer->i, 239) == 0)
                            A_PlaySound(239,pPlayer->i);
                    }
                    else
                    {
                        var104 = (pPlayer->MotoSpeed*pPlayer->MotoSpeed)>>15;
                        pPlayer->MotoSpeed -= (pPlayer->MotoSpeed>>3);
                        if (A_CheckSoundPlaying(pPlayer->i, 240) == 0)
                            A_PlaySound(240,pPlayer->i);
                    }
                    pSprite->extra -= var104;
                    if (pSprite->extra <= 0)
                    {
                        A_PlaySound(SQUISHED,pPlayer->i);
                        P_PalFrom(pPlayer,63,63,0,0);
                    }
                    else if (var104)
                        A_PlaySound(DUKE_LAND_HURT,pPlayer->i);
                }
                else if (RRRA && pPlayer->OnBoat)
                {
                    short var114, var118;
                    var114 = getangle(wall[wall[wallNum].point2].x-wall[wallNum].x,wall[wall[wallNum].point2].y-wall[wallNum].y);
                    var118 = klabs(fix16_to_int(pPlayer->q16ang)-var114);
                    switch (krand2()&1)
                    {
                        case 0:
                            pPlayer->q16ang += F16(pPlayer->MotoSpeed>>2);
                            break;
                        case 1:
                            pPlayer->q16ang -= F16(pPlayer->MotoSpeed>>2);
                            break;
                    }
                    if (var118 >= 441 && var118 <= 581)
                    {
                        pPlayer->MotoSpeed = ((pPlayer->MotoSpeed>>1)+(pPlayer->MotoSpeed>>2))>>2;
                        if (sectorLotag == 1)
                            if (A_CheckSoundPlaying(pPlayer->i, 178) == 0)
                                A_PlaySound(178,pPlayer->i);
                    }
                    else if (var118 >= 311 && var118 <= 711)
                    {
                        pPlayer->MotoSpeed -= ((pPlayer->MotoSpeed>>1)+(pPlayer->MotoSpeed>>2))>>3;
                        if (sectorLotag == 1)
                            if (A_CheckSoundPlaying(pPlayer->i, 179) == 0)
                                A_PlaySound(179,pPlayer->i);
                    }
                    else if (var118 >= 111 && var118 <= 911)
                    {
                        pPlayer->MotoSpeed -= (pPlayer->MotoSpeed>>4);
                        if (sectorLotag == 1)
                            if (A_CheckSoundPlaying(pPlayer->i, 180) == 0)
                                A_PlaySound(180,pPlayer->i);
                    }
                    else
                    {
                        pPlayer->MotoSpeed -= (pPlayer->MotoSpeed>>6);
                        if (sectorLotag == 1)
                            if (A_CheckSoundPlaying(pPlayer->i, 181) == 0)
                                A_PlaySound(181,pPlayer->i);
                    }
                }
                else
                {
                    if (wall[wallNum].lotag >= 40 && wall[wallNum].lotag <= 44)
                    {
                        if (wall[wallNum].lotag < 44)
                            dofurniture(wallNum,pPlayer->cursectnum,playerNum);
                        pushmove(&pPlayer->pos,&pPlayer->cursectnum,172L,(4L<<8),(4L<<8),CLIPMASK0);
                    }
                }
            }
            else if ((spriteNum & 49152) == 49152)
            {
                spriteNum &= (MAXSPRITES-1);
                
                if (RRRA && pPlayer->OnMotorcycle)
                {
                    if (A_CheckEnemySprite(&sprite[spriteNum]) || sprite[spriteNum].picnum == TILE_APLAYER)
                    {
                        if (sprite[spriteNum].picnum != TILE_APLAYER)
                        {
                            if (numplayers == 1)
                            {
                                vec3_t const vect = {
                                    sintable[(pPlayer->tilt_status*20+fix16_to_int(pPlayer->q16ang)+512)&2047]>>8,
                                    sintable[(pPlayer->tilt_status*20+fix16_to_int(pPlayer->q16ang))&2047]>>8,sprite[spriteNum].zvel
                                };

                                A_MoveSprite(spriteNum,&vect,CLIPMASK0);
                            }
                        }
                        else
                            actor[spriteNum].owner = pPlayer->i;
                        actor[spriteNum].picnum = TILE_MOTOHIT;
                        actor[spriteNum].extra = pPlayer->MotoSpeed>>1;
                        pPlayer->MotoSpeed -= pPlayer->MotoSpeed>>2;
                        pPlayer->moto_turb = 6;
                    }
                    else if ((sprite[spriteNum].picnum == TILE_RRTILE2431 || sprite[spriteNum].picnum == TILE_RRTILE2443 || sprite[spriteNum].picnum == TILE_RRTILE2451 || sprite[spriteNum].picnum == TILE_RRTILE2455)
                        && sprite[spriteNum].picnum != TILE_ACTIVATORLOCKED && pPlayer->MotoSpeed > 45)
                    {
                        A_PlaySound(SQUISHED,spriteNum);
                        if (sprite[spriteNum].picnum == TILE_RRTILE2431 || sprite[spriteNum].picnum == TILE_RRTILE2451)
                        {
                            if (sprite[spriteNum].lotag != 0)
                            {
                                for(bssize_t otherSprite = 0; otherSprite < MAXSPRITES; otherSprite++)
                                {
                                    if ((sprite[otherSprite].picnum == TILE_RRTILE2431 || sprite[otherSprite].picnum == TILE_RRTILE2451) && sprite[otherSprite].pal == 4)
                                    {
                                        if (sprite[spriteNum].lotag == sprite[otherSprite].lotag)
                                        {
                                            sprite[otherSprite].xrepeat = 0;
                                            sprite[otherSprite].yrepeat = 0;
                                        }
                                    }
                                }
                            }
                            A_DoGuts(spriteNum,TILE_RRTILE2460,12);
                            A_DoGuts(spriteNum,TILE_RRTILE2465,3);
                        }
                        else
                            A_DoGuts(spriteNum,TILE_RRTILE2465,3);
                        A_DoGuts(spriteNum,TILE_RRTILE2465,3);
                        sprite[spriteNum].xrepeat = 0;
                        sprite[spriteNum].yrepeat = 0;
                    }
                }
                else if (RRRA && pPlayer->OnBoat)
                {
                    if (A_CheckEnemySprite(&sprite[spriteNum]) || sprite[spriteNum].picnum == TILE_APLAYER)
                    {
                        if (sprite[spriteNum].picnum != TILE_APLAYER)
                        {
                            if (numplayers == 1)
                            {
                                vec3_t const vect = {
                                    sintable[(pPlayer->tilt_status*20+fix16_to_int(pPlayer->q16ang)+512)&2047]>>9,
                                    sintable[(pPlayer->tilt_status*20+fix16_to_int(pPlayer->q16ang))&2047]>>9,sprite[spriteNum].zvel
                                };

                                A_MoveSprite(spriteNum,&vect,CLIPMASK0);
                            }
                        }
                        else
                            actor[spriteNum].owner = pPlayer->i;
                        actor[spriteNum].picnum = TILE_MOTOHIT;
                        actor[spriteNum].extra = pPlayer->MotoSpeed>>2;
                        pPlayer->MotoSpeed -= pPlayer->MotoSpeed>>2;
                        pPlayer->moto_turb = 6;
                    }
                }
                else if (A_CheckEnemySprite(&sprite[spriteNum]))
                {
                    if (sprite[spriteNum].statnum != STAT_ACTOR)
                    {
                        actor[spriteNum].timetosleep = 0;
                        if (sprite[spriteNum].picnum == TILE_BILLYRAY)
                            A_PlaySound(404, spriteNum);
                        else
                            fi.check_fta_sounds(spriteNum);
                        changespritestat(spriteNum, STAT_ACTOR);
                    }
                }
                if (sprite[spriteNum].picnum == TILE_RRTILE3410)
                {
                    quickkill(pPlayer);
                    A_PlaySound(446, pPlayer->i);
                }
                else if (RRRA && sprite[spriteNum].picnum == TILE_RRTILE2443 && sprite[spriteNum].pal == 19)
                {
                    sprite[spriteNum].pal = 0;
                    pPlayer->DrugMode = 5;
                    pPlayer->drug_timer = (int32_t) totalclock;
                    sprite[pPlayer->i].extra = max_player_health;
                }
            }
        }
    }

    if (pPlayer->jetpack_on == 0)
    {
        if (pSprite->xvel > 16)
        {
            if (sectorLotag != ST_1_ABOVE_WATER && sectorLotag != ST_2_UNDERWATER && pPlayer->on_ground && (!RRRA || !pPlayer->sea_sick_stat))
            {
                pPlayer->pycount += 52;
                pPlayer->pycount &= 2047;
                pPlayer->pyoff   = klabs(pSprite->xvel * sintable[pPlayer->pycount]) / 1596;
            }
        }
        else if (sectorLotag != ST_2_UNDERWATER && sectorLotag != ST_1_ABOVE_WATER && (!RRRA || !pPlayer->sea_sick_stat))
            pPlayer->pyoff = 0;
    }

    pPlayer->pos.z += PHEIGHT;
    setsprite(pPlayer->i, &pPlayer->pos);
    pPlayer->pos.z -= PHEIGHT;

    if (RR)
    {
        if (sectorLotag == 800 && (!RRRA || !pPlayer->lotag800kill))
        {
            if (RRRA)
                pPlayer->lotag800kill = 1;
            quickkill(pPlayer);
            return;
        }
    }

    // ST_2_UNDERWATER
    if (pPlayer->cursectnum >= 0 && sectorLotag < 3)
    {
        usectortype const *pSector = (usectortype *)&sector[pPlayer->cursectnum];

        // TRAIN_SECTOR_TO_SE_INDEX
        if ((!ud.clipping && pSector->lotag == ST_31_TWO_WAY_TRAIN) &&
            ((unsigned)pSector->hitag < MAXSPRITES && sprite[pSector->hitag].xvel && actor[pSector->hitag].t_data[0] == 0))
        {
            quickkill(pPlayer);
            return;
        }
    }

    if ((pPlayer->cursectnum >= 0 && trueFloorDist < PHEIGHT && pPlayer->on_ground && sectorLotag != ST_1_ABOVE_WATER &&
         playerShrunk == 0 && sector[pPlayer->cursectnum].lotag == ST_1_ABOVE_WATER) && (!A_CheckSoundPlaying(pPlayer->i, DUKE_ONWATER)))
        if (!RRRA || (!pPlayer->OnBoat && !pPlayer->OnMotorcycle && sector[pPlayer->cursectnum].lotag != 321))
            A_PlaySound(DUKE_ONWATER, pPlayer->i);

    if (pPlayer->cursectnum >= 0 && pPlayer->cursectnum != pSprite->sectnum)
        changespritesect(pPlayer->i, pPlayer->cursectnum);

    if (pPlayer->cursectnum >= 0 && ud.clipping == 0)
    {
        int const squishPlayer = (pushmove((vec3_t *)pPlayer, &pPlayer->cursectnum, (!RR || pSprite->clipdist == 64) ? 164 : 16, (4L << 8), (4L << 8), CLIPMASK0) < 0 &&
                                 furthestangle(pPlayer->i, 8) < 512);

        if (squishPlayer || klabs(actor[pPlayer->i].floorz-actor[pPlayer->i].ceilingz) < (48<<8))
        {
            if (!(sector[pSprite->sectnum].lotag & 0x8000u) &&
                (isanunderoperator(sector[pSprite->sectnum].lotag) || isanearoperator(sector[pSprite->sectnum].lotag)))
                fi.activatebysector(pSprite->sectnum, pPlayer->i);

            if (squishPlayer)
            {
                quickkill(pPlayer);
                return;
            }
        }
        else if (klabs(floorZ - ceilZ) < ZOFFSET5 && isanunderoperator(sector[pPlayer->cursectnum].lotag))
            fi.activatebysector(pPlayer->cursectnum, pPlayer->i);

        if (RR && sector[pPlayer->cursectnum].ceilingz > (sector[pPlayer->cursectnum].floorz-ZOFFSET4))
        {
            quickkill(pPlayer);
            return;
        }
    }

    if (pPlayer->return_to_center > 0)
        pPlayer->return_to_center--;

    if (TEST_SYNC_KEY(playerBits, SK_CENTER_VIEW) || pPlayer->hard_landing)
        if (VM_OnEvent(EVENT_RETURNTOCENTER, pPlayer->i,playerNum) == 0)
            pPlayer->return_to_center = 9;

    if (TEST_SYNC_KEY(playerBits, SK_LOOK_UP))
    {
        if (VM_OnEvent(EVENT_LOOKUP, pPlayer->i, playerNum) == 0)
        {
            pPlayer->return_to_center = 9;
            thisPlayer.horizRecenter = true;
            thisPlayer.horizAngleAdjust = float(12<<(int)(TEST_SYNC_KEY(playerBits, SK_RUN)));
        }
    }
    else if (TEST_SYNC_KEY(playerBits, SK_LOOK_DOWN))
    {
        if (VM_OnEvent(EVENT_LOOKDOWN,pPlayer->i,playerNum) == 0)
        {
            pPlayer->return_to_center = 9;
            thisPlayer.horizRecenter = true;
            thisPlayer.horizAngleAdjust = -float(12<<(int)(TEST_SYNC_KEY(playerBits, SK_RUN)));
        }
    }
    else if (TEST_SYNC_KEY(playerBits, SK_AIM_UP) && (!RRRA || !pPlayer->OnMotorcycle))
    {
        if (VM_OnEvent(EVENT_AIMUP,pPlayer->i,playerNum) == 0)
        {
            thisPlayer.horizAngleAdjust = float(6 << (int)(TEST_SYNC_KEY(playerBits, SK_RUN)));
            thisPlayer.horizRecenter    = false;
        }
    }
    else if (TEST_SYNC_KEY(playerBits, SK_AIM_DOWN) && (!RRRA || !pPlayer->OnMotorcycle))
    {
        if (VM_OnEvent(EVENT_AIMDOWN,pPlayer->i,playerNum) == 0)
        {
            thisPlayer.horizAngleAdjust = -float(6 << (int)(TEST_SYNC_KEY(playerBits, SK_RUN)));
            thisPlayer.horizRecenter    = false;
        }
    }
    if (RR && pPlayer->recoil && *weaponFrame == 0)
    {
        int delta = pPlayer->recoil >> 1;
        if (!delta) delta++;
        pPlayer->recoil -= delta;
        pPlayer->q16horiz -= F16(delta);
    }

    if (pPlayer->hard_landing > 0)
    {
        thisPlayer.horizSkew = fix16_from_int(-(pPlayer->hard_landing << 4));
        pPlayer->hard_landing--;
    }

    //Shooting code/changes

    if (pPlayer->show_empty_weapon > 0)
    {
        --pPlayer->show_empty_weapon;

        if (!RR && pPlayer->show_empty_weapon == 0)
        {
            if (pPlayer->last_full_weapon == GROW_WEAPON)
                pPlayer->subweapon |= (1 << GROW_WEAPON);
            else if (pPlayer->last_full_weapon == SHRINKER_WEAPON)
                pPlayer->subweapon &= ~(1 << GROW_WEAPON);

            P_AddWeapon(pPlayer, pPlayer->last_full_weapon);
            return;
        }
    }

    if (RR && pPlayer->show_empty_weapon == 1)
    {
        P_AddWeapon(pPlayer, pPlayer->last_full_weapon);
        return;
    }

    if (pPlayer->knee_incs > 0)
    {
        thisPlayer.horizSkew = F16(-48);
        thisPlayer.horizRecenter = true;
        pPlayer->return_to_center = 9;

        if (++pPlayer->knee_incs > 15)
        {
            pPlayer->knee_incs      = 0;
            pPlayer->holster_weapon = 0;
            pPlayer->weapon_pos     = klabs(pPlayer->weapon_pos);

            if (pPlayer->actorsqu >= 0 && sprite[pPlayer->actorsqu].statnum != MAXSTATUS &&
                dist(&sprite[pPlayer->i], &sprite[pPlayer->actorsqu]) < 1400)
            {
                A_DoGuts(pPlayer->actorsqu, TILE_JIBS6, 7);
                fi.spawn(pPlayer->actorsqu, TILE_BLOODPOOL);
                A_PlaySound(SQUISHED, pPlayer->actorsqu);
                switch (DYNAMICTILEMAP(sprite[pPlayer->actorsqu].picnum))
                {
                    case FEM1__STATIC:
                    case FEM2__STATIC:
                    case FEM3__STATIC:
                    case FEM4__STATIC:
                    case FEM5__STATIC:
                    case FEM6__STATIC:
                    case FEM7__STATIC:
                    case FEM8__STATIC:
                    case FEM9__STATIC:
                    case PODFEM1__STATIC:
                        if (RR)
                            break;
                        fallthrough__;
                    case FEM10__STATIC:
                    case NAKED1__STATIC:
                    case STATUE__STATIC:
                        if (sprite[pPlayer->actorsqu].yvel)
                            fi.operaterespawns(sprite[pPlayer->actorsqu].yvel);
                        A_DeleteSprite(pPlayer->actorsqu);
                        break;
                    case APLAYER__STATIC:
                    {
                        int playerSquished = P_Get(pPlayer->actorsqu);
                        quickkill(g_player[playerSquished].ps);
                        g_player[playerSquished].ps->frag_ps = playerNum;
                        break;
                    }
                    default:
                        if (A_CheckEnemySprite(&sprite[pPlayer->actorsqu]))
                            P_AddKills(pPlayer, 1);
                        A_DeleteSprite(pPlayer->actorsqu);
                        break;
                }
            }
            pPlayer->actorsqu = -1;
        }
        else if (pPlayer->actorsqu >= 0)
            pPlayer->q16ang += fix16_from_int(
            getincangle(fix16_to_int(pPlayer->q16ang),
                            getangle(sprite[pPlayer->actorsqu].x - pPlayer->pos.x, sprite[pPlayer->actorsqu].y - pPlayer->pos.y))
            >> 2);
    }

    if (fi.doincrements(&ps[playerNum]))
        return;

    switch (pPlayer->weapon_pos)
    {
        case WEAPON_POS_LOWER:
            if (pPlayer->last_weapon >= 0)
            {
                pPlayer->weapon_pos  = WEAPON_POS_RAISE;
                pPlayer->last_weapon = -1;
            }
            else if (pPlayer->holster_weapon == 0)
                pPlayer->weapon_pos = WEAPON_POS_RAISE;
            break;
        case 0: break;
        default: pPlayer->weapon_pos--; break;
    }

    P_ProcessWeapon(playerNum);
}


int P_HasKey(int sectNum, int playerNum)
{
    if (g_sectorExtra[sectNum] == 0)
        return 1;
    if (g_sectorExtra[sectNum] > 6)
        return 1;

    int key = g_sectorExtra[sectNum];
    if (key > 3)
        key -= 3;

    if (g_player[playerNum].ps->keys[key] == 1)
    {
        g_sectorExtra[sectNum] = 0;
        return 1;
    }
    return 0;
}

int16_t max_ammo_amount[MAX_WEAPONS];


END_DUKE_NS
