//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

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

#include "duke3d.h"
#include "osdcmds.h"
#include "cheats.h"

// KEEPINSYNC game.h: enum cheatindex_t
char CheatStrings [NUMCHEATS][MAXCHEATLEN] =
{
    "cornholio",    // 0
    "stuff",        // 1
    "scotty###",    // 2
    "coords",       // 3
    "view",         // 4
    "time",         // 5
    "unlock",       // 6
    "cashman",      // 7
    "items",        // 8
    "rate",         // 9
    "skill#",       // 10
    "beta",         // 11
    "hyper",        // 12
    "monsters",     // 13
    "<RESERVED>",   // 14
    "<RESERVED>",   // 15
    "todd",         // 16
    "showmap",      // 17
    "kroz",         // 18
    "allen",        // 19
    "clip",         // 20
    "weapons",      // 21
    "inventory",    // 22
    "keys",         // 23
    "debug",        // 24
    "<RESERVED>",   // 25
    "<RESERVED>",   // 26
    "<RESERVED>",   // 27
    "<RESERVED>",   // 28
    "<RESERVED>",   // 29
    "<RESERVED>",   // 30
    "<RESERVED>",   // 31
    "<RESERVED>",   // 32
    "<RESERVED>",   // 33
    "<RESERVED>",   // 34
    "<RESERVED>",   // 35
    "<RESERVED>",   // 36
    "<RESERVED>",   // 37
    "<RESERVED>",   // 38
    "<RESERVED>",   // 39
};

const uint32_t CheatFunctionFlags [NUMCHEATS] =
{
    1 << CHEATFUNC_GOD,
    1 << CHEATFUNC_GIVEEVERYTHING,
    1 << CHEATFUNC_WARP,
    1 << CHEATFUNC_COORDS,
    1 << CHEATFUNC_VIEW,
    0,
    1 << CHEATFUNC_UNLOCK,
    1 << CHEATFUNC_CASHMAN,
    1 << CHEATFUNC_GIVEALLITEMS,
    1 << CHEATFUNC_FRAMERATE,
    1 << CHEATFUNC_SKILL,
    1 << CHEATFUNC_QUOTEBETA,
    1 << CHEATFUNC_HYPER,
    1 << CHEATFUNC_MONSTERS,
    0,
    0,
    1 << CHEATFUNC_QUOTETODD,
    1 << CHEATFUNC_SHOWMAP,
    1 << CHEATFUNC_GOD,
    1 << CHEATFUNC_QUOTEALLEN,
    1 << CHEATFUNC_CLIP,
    1 << CHEATFUNC_GIVEWEAPONS,
    1 << CHEATFUNC_GIVEINVENTORY,
    1 << CHEATFUNC_GIVEKEYS,
    1 << CHEATFUNC_DEBUG,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

// KEEPINSYNC game.h: enum CheatCodeFunctions
// KEEPINSYNC menus.c: MenuEntry_t ME_CheatCodes[]
const uint8_t CheatFunctionIDs[NUMCHEATS] =
{
    CHEAT_CASHMAN,
    CHEAT_CORNHOLIO,
    CHEAT_STUFF,
    CHEAT_WEAPONS,
    CHEAT_ITEMS,
    CHEAT_INVENTORY,
    CHEAT_KEYS,
    CHEAT_HYPER,
    CHEAT_VIEW,
    CHEAT_SHOWMAP,
    CHEAT_UNLOCK,
    CHEAT_CLIP,
    CHEAT_SCOTTY,
    CHEAT_SKILL,
    CHEAT_MONSTERS,
    CHEAT_RATE,
    CHEAT_BETA,
    CHEAT_TODD,
    CHEAT_ALLEN,
    CHEAT_COORDS,
    CHEAT_DEBUG,
};

char const * const g_NAMMattCheatQuote = "Matt Saettler.  matts@seanet.com";

void G_SetupCheats(void)
{
    if (RR)
    {
        CheatKeys[0] = sc_R;
        CheatKeys[1] = sc_D;
        Bstrcpy(CheatStrings[0], "hounddog");
        Bstrcpy(CheatStrings[1], "all");
        Bstrcpy(CheatStrings[2], "meadow###");
        Bstrcpy(CheatStrings[3], "yerat");
        Bstrcpy(CheatStrings[7], "cluck");
        Bstrcpy(CheatStrings[11], "teachers");
        Bstrcpy(CheatStrings[12], "moonshine");
        Bstrcpy(CheatStrings[13], "critters");
        Bstrcpy(CheatStrings[16], "rafael");
        Bstrcpy(CheatStrings[18], "elvis");
        Bstrcpy(CheatStrings[19], "<RESERVED>");
        Bstrcpy(CheatStrings[21], "guns");
        if (RRRA)
        {
            Bstrcpy(CheatStrings[25], "joseph");
            Bstrcpy(CheatStrings[26], "mrbill");
            Bstrcpy(CheatStrings[27], "tony");
            Bstrcpy(CheatStrings[28], "gary");
            Bstrcpy(CheatStrings[29], "rhett");
            Bstrcpy(CheatStrings[30], "aaron");
            Bstrcpy(CheatStrings[31], "nocheat");
            Bstrcpy(CheatStrings[32], "woleslagle");
            Bstrcpy(CheatStrings[33], "mikael");
            Bstrcpy(CheatStrings[34], "greg");
            Bstrcpy(CheatStrings[35], "noah");
            Bstrcpy(CheatStrings[36], "arijit");
            Bstrcpy(CheatStrings[37], "donut");
            Bstrcpy(CheatStrings[38], "kfc");
            Bstrcpy(CheatStrings[39], "van");
        }
    }
    else if (NAM)
    {
        CheatKeys[0] = sc_N;
        CheatKeys[1] = sc_V;

        Bstrcpy(CheatStrings[0], "acaleb");
        Bstrcpy(CheatStrings[1], "ablood");
        Bstrcpy(CheatStrings[2], "alevel###");
        Bstrcpy(CheatStrings[3], "acoords");
        Bstrcpy(CheatStrings[4], "aview");
        Bstrcpy(CheatStrings[5], "<RESERVED>");
        Bstrcpy(CheatStrings[7], "<RESERVED>");
        Bstrcpy(CheatStrings[8], "<RESERVED>");
        Bstrcpy(CheatStrings[9], "arate");
        Bstrcpy(CheatStrings[10], "askill");
        Bstrcpy(CheatStrings[11], "<RESERVED>");
        Bstrcpy(CheatStrings[12], "ahyper");
        Bstrcpy(CheatStrings[13], "<RESERVED>");
        Bstrcpy(CheatStrings[16], "amatt");
        Bstrcpy(CheatStrings[17], "ashowmap");
        Bstrcpy(CheatStrings[18], "agod");
        Bstrcpy(CheatStrings[19], "<RESERVED>");
        Bstrcpy(CheatStrings[20], "aclip");
        Bstrcpy(CheatStrings[21], "aweapons");
        Bstrcpy(CheatStrings[22], "ainventory");
        Bstrcpy(CheatStrings[23], "<RESERVED>");
        Bstrcpy(CheatStrings[24], "adebug");
        Bstrcpy(CheatStrings[26], "acgs");

        Bstrcpy(g_gametypeNames[0], "GruntMatch (Spawn)");
        Bstrcpy(g_gametypeNames[2], "GruntMatch (No Spawn)");
    }
}

static void doinvcheat(DukePlayer_t * const pPlayer, int32_t invidx, int32_t defaultnum)
{
    if (defaultnum >= 0)
        pPlayer->inv_amount[invidx] = defaultnum;
}

static void G_CheatGetInv(DukePlayer_t *pPlayer)
{
    doinvcheat(pPlayer, GET_STEROIDS, 400);
    if (!RR) doinvcheat(pPlayer, GET_HEATS, 1200);
    doinvcheat(pPlayer, GET_BOOTS, RR ? 2000 : 200);
    doinvcheat(pPlayer, GET_SHIELD, 100);
    doinvcheat(pPlayer, GET_SCUBA, 6400);
    doinvcheat(pPlayer, GET_HOLODUKE, 2400);
    doinvcheat(pPlayer, GET_JETPACK, RR ? 600 : 1600);
    doinvcheat(pPlayer, GET_FIRSTAID, pPlayer->max_player_health);
}

static void end_cheat(DukePlayer_t * const pPlayer)
{
    pPlayer->cheat_phase = 0;
    KB_FlushKeyboardQueue();
}

static int32_t cheatbuflen;
static int8_t cheatbuf[MAXCHEATLEN];

void G_DoCheats(void)
{
    DukePlayer_t * const pPlayer = g_player[myconnectindex].ps;
    int consoleCheat = 0;
    int cheatNum;

    if (osdcmd_cheatsinfo_stat.cheatnum != -1)
    {
        cheatNum = osdcmd_cheatsinfo_stat.cheatnum;

        if (ud.player_skill == 4 || (RR && ud.player_skill > 3) || (RRRA && pPlayer->nocheat))
        {
            switch (cheatNum)
            {
            case CHEAT_DEBUG:
            case CHEAT_COORDS:
            case CHEAT_RATE:
            case CHEAT_RESERVED:
            case CHEAT_RESERVED2:
            //case CHEAT_RESERVED3:
                break;
            default:
                P_DoQuote(QUOTE_CHEATS_DISABLED, pPlayer);
                osdcmd_cheatsinfo_stat.cheatnum = -1;
                return;
            }
        }

        // JBF 20030914
        osdcmd_cheatsinfo_stat.cheatnum = -1;
        consoleCheat = 1;
    }

    static int volumeOne = 0;

    if (VOLUMEONE && !volumeOne)
    {
        // change "scotty###" to "scotty##"
        uint32_t const warpend = Bstrlen(CheatStrings[2]);
        if (strcmp(&CheatStrings[2][warpend-3], "###") == 0)
            CheatStrings[2][warpend-1] = '\0';

        Bstrcpy(CheatStrings[6], "<RESERVED>");
        volumeOne = 1;
    }

    if (consoleCheat && numplayers < 2 && ud.recstat == 0)
        goto FOUNDCHEAT;

    if ((RR && ud.player_skill > 3) || (RRRA && pPlayer->nocheat))
        return;

    if (pPlayer->gm & (MODE_TYPE|MODE_MENU))
        return;

    if (pPlayer->cheat_phase == 1)
    {
        int ch;

        while (KB_KeyWaiting())
        {
            ch = Btolower(KB_GetCh());

            if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')))
            {
                pPlayer->cheat_phase = 0;
                //                P_DoQuote(QUOTE_46,pPlayer);
                return;
            }

            cheatbuf[cheatbuflen++] = (int8_t) ch;
            // This assertion is not obvious, but it should hold because of the
            // cheat string matching logic below.
            Bassert(cheatbuflen < (signed)sizeof(cheatbuf));
            cheatbuf[cheatbuflen] = 0;
            //            KB_ClearKeysDown();

            for (cheatNum=0; cheatNum < NUMCHEATCODES; cheatNum++)
            {
                for (bssize_t j = 0; j<cheatbuflen; j++)
                {
                    if (cheatbuf[j] == CheatStrings[cheatNum][j] || (CheatStrings[cheatNum][j] == '#' && ch >= '0' && ch <= '9'))
                    {
                        if (CheatStrings[cheatNum][j+1] == 0) goto FOUNDCHEAT;
                        if (j == cheatbuflen-1) return;
                    }
                    else break;
                }
            }

            pPlayer->cheat_phase = 0;
            return;

        FOUNDCHEAT:;

            if (cheatNum == CHEAT_SCOTTY)
            {
                size_t const i = Bstrlen(CheatStrings[cheatNum])-3+VOLUMEONE;
                if (!consoleCheat)
                {
                    // JBF 20030914
                    int32_t volnume, levnume;
                    if (VOLUMEALL)
                    {
                        volnume = cheatbuf[i] - '0';
                        levnume = (cheatbuf[i+1] - '0')*10+(cheatbuf[i+2]-'0');
                    }
                    else
                    {
                        volnume = cheatbuf[i] - '0';
                        levnume = cheatbuf[i+1] - '0';
                    }

                    volnume--;
                    levnume--;

                    ud.m_volume_number = volnume;
                    ud.m_level_number = levnume;
                }
                else
                {
                    // JBF 20030914
                    ud.m_volume_number = osdcmd_cheatsinfo_stat.volume;
                    ud.m_level_number = osdcmd_cheatsinfo_stat.level;
                }
            }
            else if (cheatNum == CHEAT_SKILL)
            {
                if (!consoleCheat)
                {
                    size_t const i = Bstrlen(CheatStrings[cheatNum])-1;
                    ud.m_player_skill = cheatbuf[i] - '1';
                }
                else
                {
                    ud.m_player_skill = osdcmd_cheatsinfo_stat.volume;
                }
            }

            {
                switch (cheatNum)
                {
                case CHEAT_WEAPONS:
                {
                    int const weaponLimit = (VOLUMEONE) ? 6 : 0;

                    for (bssize_t weaponNum = PISTOL_WEAPON; weaponNum < MAX_WEAPONS-weaponLimit; weaponNum++)
                    {
                        P_AddAmmo(pPlayer, weaponNum, pPlayer->max_ammo_amount[weaponNum]);
                        pPlayer->gotweapon |= (1<<weaponNum);
                    }

                    if (RRRA) pPlayer->ammo_amount[SLINGBLADE_WEAPON] = 1;

                    P_DoQuote(QUOTE_CHEAT_ALL_WEAPONS, pPlayer);

                    end_cheat(pPlayer);
                }
                    return;

                case CHEAT_INVENTORY:
                    G_CheatGetInv(pPlayer);
                    P_DoQuote(QUOTE_CHEAT_ALL_INV, pPlayer);
                    end_cheat(pPlayer);
                    return;

                case CHEAT_KEYS:
                    pPlayer->got_access =  7;
                    if (RR)
                        for (int key = 0; key < 5; key++)
                            pPlayer->keys[key] = 1;
                    KB_FlushKeyboardQueue();
                    P_DoQuote(QUOTE_CHEAT_ALL_KEYS, pPlayer);
                    end_cheat(pPlayer);
                    return;

                case CHEAT_DEBUG:
                    g_Debug = 1-g_Debug;

                    G_DumpDebugInfo();
                    Bsprintf(tempbuf, "Gamevars dumped to log");
                    G_AddUserQuote(tempbuf);
                    Bsprintf(tempbuf, "Map dumped to debug.map");
                    G_AddUserQuote(tempbuf);
                    end_cheat(pPlayer);
                    break;

                case CHEAT_CLIP:
                    ud.noclip = !ud.noclip;
                    P_DoQuote(QUOTE_CHEAT_NOCLIP-!ud.noclip, pPlayer);
                    end_cheat(pPlayer);
                    return;

                case CHEAT_RESERVED2:
                    if (RR)
                    {
                        P_DoQuote(QUOTE_JETPACK_ON, pPlayer);
                        KB_FlushKeyboardQueue();
                    }
                    else
                    {
                        pPlayer->player_par = 0;
                        pPlayer->gm = MODE_EOL;
                    }
                    end_cheat(pPlayer);
                    return;

                case CHEAT_ALLEN:
                    P_DoQuote(QUOTE_CHEAT_ALLEN, pPlayer);
                    pPlayer->cheat_phase = 0;
                    KB_ClearKeyDown(sc_N);
                    return;

                case CHEAT_CORNHOLIO:
                case CHEAT_KROZ:
                //case CHEAT_COMEGETSOME:
                {
                    const int32_t pi = pPlayer->i;

                    ud.god = 1-ud.god;

                    if (ud.god)
                    {
                        if (RRRA)
                            S_PlaySound(218);
                        pus = 1;
                        pub = 1;
                        sprite[pi].cstat = 257;

                        actor[pi].t_data[0] = 0;
                        actor[pi].t_data[1] = 0;
                        actor[pi].t_data[2] = 0;
                        actor[pi].t_data[3] = 0;
                        actor[pi].t_data[4] = 0;
                        actor[pi].t_data[5] = 0;

                        sprite[pi].hitag = 0;
                        sprite[pi].lotag = 0;
                        sprite[pi].pal = pPlayer->palookup;

                        //if (cheatNum != CHEAT_COMEGETSOME)
                        //{
                            P_DoQuote(QUOTE_CHEAT_GODMODE_ON, pPlayer);
                        //}
                        //else
                        //{
                        //    Bstrcpy(apStrings[QUOTE_RESERVED4], "Come Get Some!");
                        //
                        //    S_PlaySound(DUKE_GETWEAPON2);
                        //    P_DoQuote(QUOTE_RESERVED4, pPlayer);
                        //    G_CheatGetInv(pPlayer);
                        //
                        //    for (bssize_t weaponNum = PISTOL_WEAPON; weaponNum < MAX_WEAPONS; weaponNum++)
                        //        pPlayer->gotweapon |= (1<<weaponNum);
                        //
                        //    for (bssize_t weaponNum = PISTOL_WEAPON; weaponNum < MAX_WEAPONS; weaponNum++)
                        //        P_AddAmmo(pPlayer, weaponNum, pPlayer->max_ammo_amount[weaponNum]);
                        //
                        //    pPlayer->got_access = 7;
                        //}
                    }
                    else
                    {
                        sprite[pi].extra = pPlayer->max_player_health;
                        actor[pi].extra = -1;
                        pPlayer->last_extra = pPlayer->max_player_health;
                        P_DoQuote(QUOTE_CHEAT_GODMODE_OFF, pPlayer);
                    }

                    sprite[pi].extra = pPlayer->max_player_health;
                    actor[pi].extra = 0;

                    //if (cheatNum != CHEAT_COMEGETSOME)
                        pPlayer->dead_flag = 0;

                    end_cheat(pPlayer);
                    return;
                }

                case CHEAT_STUFF:
                {
                    int const weaponLimit = (VOLUMEONE) ? 6 : 0;

                    for (bssize_t weaponNum = PISTOL_WEAPON; weaponNum < MAX_WEAPONS-weaponLimit; weaponNum++)
                        pPlayer->gotweapon |= (1<<weaponNum);

                    for (bssize_t weaponNum = PISTOL_WEAPON; weaponNum < MAX_WEAPONS-weaponLimit; weaponNum++)
                        P_AddAmmo(pPlayer, weaponNum, pPlayer->max_ammo_amount[weaponNum]);

                    if (RRRA)
                        pPlayer->ammo_amount[SLINGBLADE_WEAPON] = 1;

                    G_CheatGetInv(pPlayer);
                    pPlayer->got_access = 7;
                    if (RR)
                        for (int key = 0; key < 5; key++)
                            pPlayer->keys[key] = 1;
                    P_DoQuote(QUOTE_CHEAT_EVERYTHING, pPlayer);

                    //                    P_DoQuote(QUOTE_21,pPlayer);
                    pPlayer->inven_icon = ICON_FIRSTAID;

                    end_cheat(pPlayer);
                    return;
                }

                case CHEAT_SCOTTY:
                {
                    if (RR)
                        g_lastLevel = 0;
                    int32_t const volnume = ud.m_volume_number, levnume = ud.m_level_number;

                    if ((!VOLUMEONE || volnume == 0) && (unsigned)volnume < (unsigned)g_volumeCnt &&
                        (unsigned)levnume < MAXLEVELS && g_mapInfo[volnume*MAXLEVELS + levnume].filename != NULL)
                    {
                        ud.volume_number = volnume;
                        ud.level_number = levnume;

#if 0
                        if (numplayers > 1 && g_netServer)
                            Net_NewGame(volnume, levnume);
                        else
#endif
                            pPlayer->gm |= MODE_RESTART;
                    }

                    end_cheat(pPlayer);
                    return;
                }

                case CHEAT_SKILL:
                    if (RR)
                        g_lastLevel = 0;
                    ud.player_skill = ud.m_player_skill;

#if 0
                    if (numplayers > 1 && g_netServer)
                        Net_NewGame(ud.m_volume_number, ud.m_level_number);
                    else
#endif
                        pPlayer->gm |= MODE_RESTART;

                    end_cheat(pPlayer);
                    return;

                case CHEAT_COORDS:
#ifdef USE_OPENGL
                    if (++ud.coords >= 3) ud.coords = 0;
#else
                    if (++ud.coords >= 2) ud.coords = 0;
#endif
                    end_cheat(pPlayer);
                    return;

                case CHEAT_VIEW:
                    if (!RRRA || (!pPlayer->on_motorcycle && !pPlayer->on_boat))
                    {
                        pPlayer->over_shoulder_on ^= 1;
                        CAMERADIST = 0;
                        CAMERACLOCK = (int32_t) totalclock;
                        //                    P_DoQuote(QUOTE_CHEATS_DISABLED,pPlayer);
                    }
                    end_cheat(pPlayer);
                    return;

                case CHEAT_TIME:
                    //                    P_DoQuote(QUOTE_21,pPlayer);
                    end_cheat(pPlayer);
                    return;

                case CHEAT_UNLOCK:
                    if (VOLUMEONE) return;

                    for (bssize_t i=numsectors-1; i>=0; i--) //Unlock
                    {
                        int const lotag = sector[i].lotag;
                        if (lotag == -1 || lotag == 32767) continue;
                        if ((lotag & 0x7fff) > 2)
                        {
                            if (lotag & (uint16_t)~16384u)
                                sector[i].lotag &= (uint16_t)~16384u;
                            G_OperateSectors(i, pPlayer->i);
                        }
                    }
                    G_OperateForceFields(pPlayer->i, -1);

                    P_DoQuote(QUOTE_CHEAT_UNLOCK, pPlayer);
                    end_cheat(pPlayer);
                    return;

                case CHEAT_CASHMAN:
                    ud.cashman = 1-ud.cashman;
                    KB_ClearKeyDown(sc_N);
                    pPlayer->cheat_phase = 0;
                    return;

                case CHEAT_ITEMS:
                    G_CheatGetInv(pPlayer);
                    pPlayer->got_access = 7;
                    if (RR)
                        for(int key = 0; key < 5; key++)
                            pPlayer->keys[key] = 1;
                    P_DoQuote(QUOTE_CHEAT_EVERYTHING, pPlayer);
                    end_cheat(pPlayer);
                    return;

                case CHEAT_SHOWMAP: // SHOW ALL OF THE MAP TOGGLE;
                    ud.showallmap = !ud.showallmap;

                    for (char & i : show2dsector)
                        i = ud.showallmap*255;

                    P_DoQuote(ud.showallmap ? QUOTE_SHOW_MAP_ON : QUOTE_SHOW_MAP_OFF,
                        pPlayer);

                    end_cheat(pPlayer);
                    return;

                case CHEAT_TODD:
                    if (NAM)
                    {
                        Bstrcpy(apStrings[QUOTE_RESERVED4], g_NAMMattCheatQuote);
                        P_DoQuote(QUOTE_RESERVED4, pPlayer);
                    }
                    else
                    {
                        P_DoQuote(QUOTE_CHEAT_TODD, pPlayer);
                    }

                    end_cheat(pPlayer);
                    return;

                case CHEAT_RATE:
                    if (++ud.showfps > 3)
                        ud.showfps = 0;

                    end_cheat(pPlayer);
                    return;

                case CHEAT_BETA:
                    P_DoQuote(QUOTE_CHEAT_BETA, pPlayer);
                    KB_ClearKeyDown(sc_H);
                    end_cheat(pPlayer);
                    return;

                case CHEAT_HYPER:
                    pPlayer->inv_amount[GET_STEROIDS] = 399;
                    if (!RR)
                        pPlayer->inv_amount[GET_HEATS] = 1200;
                    P_DoQuote(QUOTE_CHEAT_STEROIDS, pPlayer);
                    end_cheat(pPlayer);
                    return;

                case CHEAT_MONSTERS:
                {
                    const char *s [] = { "On", "Off", "On (2)" };

                    if (++g_noEnemies == 3)
                        g_noEnemies = 0;

                    Bsprintf(apStrings[QUOTE_RESERVED4], "Monsters: %s", s[g_noEnemies]);
                    P_DoQuote(QUOTE_RESERVED4, pPlayer);

                    end_cheat(pPlayer);
                    return;
                }

                case CHEAT_RESERVED:
                //case CHEAT_RESERVED3:
                    if (RR)
                    {
                        P_DoQuote(51, pPlayer);
                        end_cheat(pPlayer);
                    }
                    else
                    {
                        ud.eog = 1;
                        pPlayer->player_par = 0;
                        pPlayer->gm |= MODE_EOL;
                    }
                    KB_FlushKeyboardQueue();
                    return;

                case CHEAT_RAJOSEPH:
                    G_OnMotorcycle(pPlayer, 0);
                    pPlayer->ammo_amount[MOTORCYCLE_WEAPON] = pPlayer->max_ammo_amount[MOTORCYCLE_WEAPON];
                    P_DoQuote(126, pPlayer);
                    end_cheat(pPlayer);
                    KB_FlushKeyboardQueue();
                    return;

                case CHEAT_RAMRBILL:
                    P_QuickKill(pPlayer);
                    P_DoQuote(127, pPlayer);
                    end_cheat(pPlayer);
                    KB_FlushKeyboardQueue();
                    return;

                case CHEAT_RAGARY:
                    S_PlayRRMusic(10);
                    end_cheat(pPlayer);
                    KB_FlushKeyboardQueue();
                    return;

                case CHEAT_RANOAH:
                    end_cheat(pPlayer);
                    KB_FlushKeyboardQueue();
                    return;

                case CHEAT_RARHETT:
                    ud.god = 0;
                    pPlayer->gotweapon = 1<<KNEE_WEAPON;
                    pPlayer->curr_weapon = KNEE_WEAPON;
                    pPlayer->nocheat = 1;
                    sprite[pPlayer->i].extra = 1;
                    P_DoQuote(128, pPlayer);
                    end_cheat(pPlayer);
                    KB_FlushKeyboardQueue();
                    return;

                case CHEAT_RAAARON:
                    pPlayer->drug_mode = pPlayer->drug_mode ? 0 : 5;
                    pPlayer->drug_timer = (int32_t) totalclock;
                    end_cheat(pPlayer);
                    KB_FlushKeyboardQueue();
                    return;

                case CHEAT_RANOCHEAT:
                    pPlayer->nocheat = 1;
                    P_DoQuote(130, pPlayer);
                    end_cheat(pPlayer);
                    KB_FlushKeyboardQueue();
                    return;

                case CHEAT_RATONY:
                    g_changeEnemySize = 2;
                    end_cheat(pPlayer);
                    KB_FlushKeyboardQueue();
                    return;

                case CHEAT_RAVAN:
                    g_changeEnemySize = 3;
                    end_cheat(pPlayer);
                    KB_FlushKeyboardQueue();
                    return;

                case CHEAT_RAKFC:
                    for (int i = 0; i < 7; i++)
                    {
                        int const newSprite = A_Spawn(pPlayer->i, HEN);
                        sprite[newSprite].pal = 1;
                        sprite[newSprite].xrepeat <<= 2;
                        sprite[newSprite].yrepeat <<= 2;
                    }
                    P_DoQuote(139, pPlayer);
                    end_cheat(pPlayer);
                    KB_FlushKeyboardQueue();
                    return;

                case CHEAT_RAWOLESLAGLE:
                    if (pPlayer->drink_amt)
                    {
                        pPlayer->drink_amt = 0;
                        P_DoQuote(132, pPlayer);
                    }
                    else
                    {
                        pPlayer->drink_amt = 90;
                        P_DoQuote(131, pPlayer);
                    }
                    end_cheat(pPlayer);
                    KB_FlushKeyboardQueue();
                    return;

                case CHEAT_RAMIKAEL:
                    for (bssize_t weaponNum = PISTOL_WEAPON; weaponNum < MAX_WEAPONS; weaponNum++)
                    {
                        pPlayer->gotweapon |= 1 << weaponNum;
                        pPlayer->ammo_amount[weaponNum] = 66;
                    }

                    pPlayer->ammo_amount[SLINGBLADE_WEAPON] = 1;

                    G_CheatGetInv(pPlayer);
                    pPlayer->got_access = 7;
                    for (int key = 0; key < 5; key++)
                        pPlayer->keys[key] = 1;
                    P_DoQuote(5, pPlayer);
                    end_cheat(pPlayer);
                    KB_FlushKeyboardQueue();
                    return;

                case CHEAT_RAGREG:
                    if (pPlayer->sea_sick_stat)
                    {
                        pPlayer->sea_sick_stat = 0;
                        P_DoQuote(129, pPlayer);
                    }
                    else
                    {
                        pPlayer->sea_sick_stat = 1;
                        P_DoQuote(137, pPlayer);
                    }
                    end_cheat(pPlayer);
                    KB_FlushKeyboardQueue();
                    return;

                case CHEAT_RAARIJIT:
                case CHEAT_RADONUT:
                    G_OnBoat(pPlayer, 0);
                    pPlayer->ammo_amount[BOAT_WEAPON] = pPlayer->max_ammo_amount[BOAT_WEAPON];
                    P_DoQuote(136, pPlayer);
                    end_cheat(pPlayer);
                    KB_FlushKeyboardQueue();
                    return;

                default:
                    end_cheat(pPlayer);
                    return;
                }
            }
        }
    }
    else
    {
        if (KB_KeyPressed((uint8_t) CheatKeys[0]))
        {
            if (pPlayer->cheat_phase >= 0 && numplayers < 2 && ud.recstat == 0)
            {
                if (CheatKeys[0] == CheatKeys[1])
                    KB_ClearKeyDown((uint8_t) CheatKeys[0]);
                pPlayer->cheat_phase = -1;
            }
        }

        if (KB_KeyPressed((uint8_t) CheatKeys[1]))
        {
            if (pPlayer->cheat_phase == -1)
            {
                if (ud.player_skill == 4)
                {
                    P_DoQuote(QUOTE_CHEATS_DISABLED, pPlayer);
                    pPlayer->cheat_phase = 0;
                }
                else
                {
                    pPlayer->cheat_phase = 1;
                    //                    P_DoQuote(QUOTE_25,pPlayer);
                    cheatbuflen = 0;
                }
                KB_FlushKeyboardQueue();
            }
            else if (pPlayer->cheat_phase != 0)
            {
                pPlayer->cheat_phase = 0;
                KB_ClearKeyDown((uint8_t) CheatKeys[0]);
                KB_ClearKeyDown((uint8_t) CheatKeys[1]);
            }
        }
    }
}
