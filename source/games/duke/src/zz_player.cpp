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
                weaponPal, 34816, pPlayer->TiltStatus * 5 + (pPlayer->TiltStatus < 0 ? 2047 : 0));
            return;
        }
        if (pPlayer->OnBoat)
        {
            int boatTile;
            if (pPlayer->TiltStatus > 0)
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
            else if (pPlayer->TiltStatus < 0)
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

            if (pPlayer->NotOnWater)
                weaponY = 170;
            else
                weaponY = 170 + (*weaponFrame>>2);

            G_DrawTileScaled(160-(pPlayer->look_ang>>1), weaponY, boatTile, weaponShade, 2 | DRAWEAP_CENTER,
                weaponPal, 66048, pPlayer->TiltStatus + (pPlayer->TiltStatus < 0 ? 2047 : 0));
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

    if (pPlayer->curr_weapon > 11) return 0;

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
            pPlayer->TiltStatus--;
            if (pPlayer->TiltStatus < -10)
                pPlayer->TiltStatus = -10;
        }
        else if (turnRight)
        {
            pPlayer->TiltStatus++;
            if (pPlayer->TiltStatus > 10)
                pPlayer->TiltStatus = 10;
        }
    }
    else
    {
        if (turnLeft || pPlayer->moto_drink < 0)
        {
            turnHeldTime += elapsedTics;
            pPlayer->TiltStatus--;
            if (pPlayer->TiltStatus < -10)
                pPlayer->TiltStatus = -10;
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
            pPlayer->TiltStatus++;
            if (pPlayer->TiltStatus > 10)
                pPlayer->TiltStatus = 10;
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

            if (pPlayer->TiltStatus > 0)
                pPlayer->TiltStatus--;
            else if (pPlayer->TiltStatus < 0)
                pPlayer->TiltStatus++;
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
            if (!pPlayer->NotOnWater)
            {
                pPlayer->TiltStatus--;
                if (pPlayer->TiltStatus < -10)
                    pPlayer->TiltStatus = -10;
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
            if (!pPlayer->NotOnWater)
            {
                pPlayer->TiltStatus++;
                if (pPlayer->TiltStatus > 10)
                    pPlayer->TiltStatus = 10;
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
        else if (!pPlayer->NotOnWater)
        {
            turnHeldTime = 0;

            if (pPlayer->TiltStatus > 0)
                pPlayer->TiltStatus--;
            else if (pPlayer->TiltStatus < 0)
                pPlayer->TiltStatus++;
        }
    }
    else if (!pPlayer->NotOnWater)
    {
        turnHeldTime = 0;

        if (pPlayer->TiltStatus > 0)
            pPlayer->TiltStatus--;
        else if (pPlayer->TiltStatus < 0)
            pPlayer->TiltStatus++;
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

int16_t WeaponPickupSprites[MAX_WEAPONS] = { KNEE__STATIC, FIRSTGUNSPRITE__STATIC, SHOTGUNSPRITE__STATIC,
        CHAINGUNSPRITE__STATIC, RPGSPRITE__STATIC, HEAVYHBOMB__STATIC, SHRINKERSPRITE__STATIC, DEVISTATORSPRITE__STATIC,
        TRIPBOMBSPRITE__STATIC, FREEZESPRITE__STATIC, HEAVYHBOMB__STATIC, SHRINKERSPRITE__STATIC
                                           };

void P_AddAmmo(DukePlayer_t * const pPlayer, int const weaponNum, int const addAmount)
{
    pPlayer->ammo_amount[weaponNum] += addAmount;

    if (pPlayer->ammo_amount[weaponNum] > max_ammo_amount[weaponNum])
        pPlayer->ammo_amount[weaponNum] = max_ammo_amount[weaponNum];
}

void P_AddWeapon(DukePlayer_t *pPlayer, int weaponNum)
{
    fi.addweapon(pPlayer, weaponNum);
}


void P_CheckWeapon(DukePlayer_t *pPlayer)
{
    checkavailweapon(pPlayer);
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


END_DUKE_NS
