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
char CheatStrings [][MAXCHEATLEN] =
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
    "cgs",          // 26
};

const uint32_t CheatFunctionFlags [] =
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
    (1 << CHEATFUNC_GOD) | (1 << CHEATFUNC_GIVEEVERYTHING),
};

// KEEPINSYNC game.h: enum CheatCodeFunctions
// KEEPINSYNC menus.c: MenuEntry_t ME_CheatCodes[]
const uint8_t CheatFunctionIDs [] =
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

char const * const g_NAMMattCheatQuote = "Matt Saettler.  matts@saettler.com";

void G_SetupCheats(void)
{
    // KEEPINSYNC: NAM_WW2GI_CHEATS
    if (WW2GI)
    {
#if 0
        // WWII GI's original cheat prefix temporarily disabled because W conflicts with WSAD movement
        CheatKeys[0] = CheatKeys[1] = sc_W;
#else
        CheatKeys[0] = sc_G;
        CheatKeys[1] = sc_I;
#endif

        Bstrcpy(CheatStrings[0], "2god");
        Bstrcpy(CheatStrings[1], "2blood");
        Bstrcpy(CheatStrings[2], "2level###");
        Bstrcpy(CheatStrings[3], "2coords");
        Bstrcpy(CheatStrings[4], "2view");
        Bstrcpy(CheatStrings[5], "<RESERVED>");
        Bstrcpy(CheatStrings[7], "<RESERVED>");
        Bstrcpy(CheatStrings[8], "<RESERVED>");
        Bstrcpy(CheatStrings[9], "2rate");
        Bstrcpy(CheatStrings[10], "2skill");
        Bstrcpy(CheatStrings[11], "<RESERVED>");
        Bstrcpy(CheatStrings[12], "<RESERVED>");
        Bstrcpy(CheatStrings[13], "<RESERVED>");
        Bstrcpy(CheatStrings[16], "2matt");
        Bstrcpy(CheatStrings[17], "2showmap");
        Bstrcpy(CheatStrings[18], "2ryan");
        Bstrcpy(CheatStrings[19], "<RESERVED>");
        Bstrcpy(CheatStrings[20], "2clip");
        Bstrcpy(CheatStrings[21], "2weapons");
        Bstrcpy(CheatStrings[22], "2inventory");
        Bstrcpy(CheatStrings[23], "<RESERVED>");
        Bstrcpy(CheatStrings[24], "2debug");
        Bstrcpy(CheatStrings[26], "2cgs");

        Bstrcpy(GametypeNames[0], "GI Match (Spawn)");
        Bstrcpy(GametypeNames[2], "GI Match (No Spawn)");
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

        Bstrcpy(GametypeNames[0], "GruntMatch (Spawn)");
        Bstrcpy(GametypeNames[2], "GruntMatch (No Spawn)");
    }
}


static void doinvcheat(int32_t invidx, int32_t defaultnum, int32_t event)
{
    defaultnum = VM_OnEventWithReturn(event, g_player[myconnectindex].ps->i, myconnectindex, defaultnum);
    if (defaultnum >= 0)
        g_player[myconnectindex].ps->inv_amount[invidx] = defaultnum;
}

static void G_CheatGetInv(void)
{
    doinvcheat(GET_STEROIDS, 400, EVENT_CHEATGETSTEROIDS);
    doinvcheat(GET_HEATS, 1200, EVENT_CHEATGETHEAT);
    doinvcheat(GET_BOOTS, 200, EVENT_CHEATGETBOOT);
    doinvcheat(GET_SHIELD, 100, EVENT_CHEATGETSHIELD);
    doinvcheat(GET_SCUBA, 6400, EVENT_CHEATGETSCUBA);
    doinvcheat(GET_HOLODUKE, 2400, EVENT_CHEATGETHOLODUKE);
    doinvcheat(GET_JETPACK, 1600, EVENT_CHEATGETJETPACK);
    doinvcheat(GET_FIRSTAID, g_player[myconnectindex].ps->max_player_health, EVENT_CHEATGETFIRSTAID);
}

static void end_cheat(void)
{
    g_player[myconnectindex].ps->cheat_phase = 0;
    KB_FlushKeyboardQueue();
}

static int32_t cheatbuflen;
static int8_t cheatbuf[MAXCHEATLEN];

void G_DoCheats(void)
{
    int32_t ch, i, j, k=0, weapon;
    static int32_t vol1inited=0;
    char consolecheat = 0;  // JBF 20030914

    if (osdcmd_cheatsinfo_stat.cheatnum != -1)
    {
        if (ud.player_skill == 4)
        {
            P_DoQuote(QUOTE_CHEATS_DISABLED, g_player[myconnectindex].ps);
            osdcmd_cheatsinfo_stat.cheatnum = -1;
            return;
        }

        // JBF 20030914
        k = osdcmd_cheatsinfo_stat.cheatnum;
        osdcmd_cheatsinfo_stat.cheatnum = -1;
        consolecheat = 1;
    }

    if (VOLUMEONE && !vol1inited)
    {
        // change "scotty###" to "scotty##"
        uint32_t const warpend = Bstrlen(CheatStrings[2]);
        if (strcmp(&CheatStrings[2][warpend-3], "###") == 0)
            CheatStrings[2][warpend-1] = '\0';

        Bstrcpy(CheatStrings[6], "<RESERVED>");
        vol1inited = 1;
    }

    if (consolecheat && numplayers < 2 && ud.recstat == 0)
        goto FOUNDCHEAT;

    if (g_player[myconnectindex].ps->gm & (MODE_TYPE|MODE_MENU))
        return;

    if (g_player[myconnectindex].ps->cheat_phase == 1)
    {
        while (KB_KeyWaiting())
        {
            ch = Btolower(KB_GetCh());

            if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')))
            {
                g_player[myconnectindex].ps->cheat_phase = 0;
                //                P_DoQuote(QUOTE_46,g_player[myconnectindex].ps);
                return;
            }

            cheatbuf[cheatbuflen++] = (int8_t) ch;
            // This assertion is not obvious, but it should hold because of the
            // cheat string matching logic below.
            Bassert(cheatbuflen < (signed)sizeof(cheatbuf));
            cheatbuf[cheatbuflen] = 0;
            //            KB_ClearKeysDown();

            for (k=0; k < NUMCHEATCODES; k++)
            {
                for (j = 0; j<cheatbuflen; j++)
                {
                    if (cheatbuf[j] == CheatStrings[k][j] || (CheatStrings[k][j] == '#' && ch >= '0' && ch <= '9'))
                    {
                        if (CheatStrings[k][j+1] == 0) goto FOUNDCHEAT;
                        if (j == cheatbuflen-1) return;
                    }
                    else break;
                }
            }

            g_player[myconnectindex].ps->cheat_phase = 0;
            return;

        FOUNDCHEAT:

            i = VM_OnEventWithReturn(EVENT_ACTIVATECHEAT, g_player[myconnectindex].ps->i, myconnectindex, k);
            if (k != CHEAT_COMEGETSOME) // Users are not allowed to interfere with TX's debugging cheat.
                k = i;

            {
                switch (k)
                {
                case CHEAT_WEAPONS:
                    j = 0;

                    if (VOLUMEONE)
                        j = 6;

                    for (weapon = PISTOL_WEAPON; weapon < MAX_WEAPONS-j; weapon++)
                    {
                        P_AddAmmo(g_player[myconnectindex].ps, weapon, g_player[myconnectindex].ps->max_ammo_amount[weapon]);
                        g_player[myconnectindex].ps->gotweapon |= (1<<weapon);
                    }

                    P_DoQuote(QUOTE_CHEAT_ALL_WEAPONS, g_player[myconnectindex].ps);

                    end_cheat();
                    return;

                case CHEAT_INVENTORY:
                    G_CheatGetInv();
                    P_DoQuote(QUOTE_CHEAT_ALL_INV, g_player[myconnectindex].ps);
                    end_cheat();
                    return;

                case CHEAT_KEYS:
                    g_player[myconnectindex].ps->got_access =  7;
                    KB_FlushKeyboardQueue();
                    P_DoQuote(QUOTE_CHEAT_ALL_KEYS, g_player[myconnectindex].ps);
                    end_cheat();
                    return;

                case CHEAT_DEBUG:
                    g_Debug = 1-g_Debug;

                    G_DumpDebugInfo();
                    Bsprintf(tempbuf, "Gamevars dumped to log");
                    G_AddUserQuote(tempbuf);
                    Bsprintf(tempbuf, "Map dumped to debug.map");
                    G_AddUserQuote(tempbuf);
                    end_cheat();
                    break;

                case CHEAT_CLIP:
                    ud.noclip = !ud.noclip;
                    P_DoQuote(QUOTE_CHEAT_NOCLIP-!ud.noclip, g_player[myconnectindex].ps);
                    end_cheat();
                    return;

                case CHEAT_RESERVED2:
                    g_player[myconnectindex].ps->player_par = 0;
                    g_player[myconnectindex].ps->gm = MODE_EOL;
                    end_cheat();
                    return;

                case CHEAT_ALLEN:
                    P_DoQuote(QUOTE_CHEAT_ALLEN, g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    KB_ClearKeyDown(sc_N);
                    return;

                case CHEAT_CORNHOLIO:
                case CHEAT_KROZ:
                case CHEAT_COMEGETSOME:
                {
                    const int32_t pi = g_player[myconnectindex].ps->i;

                    ud.god = 1-ud.god;

                    if (ud.god)
                    {
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
                        sprite[pi].pal = g_player[myconnectindex].ps->palookup;

                        if (k != CHEAT_COMEGETSOME)
                        {
                            P_DoQuote(QUOTE_CHEAT_GODMODE_ON, g_player[myconnectindex].ps);
                        }
                        else
                        {
                            Bstrcpy(apStrings[QUOTE_RESERVED4], "Come Get Some!");

                            S_PlaySound(DUKE_GETWEAPON2);
                            P_DoQuote(QUOTE_RESERVED4, g_player[myconnectindex].ps);
                            G_CheatGetInv();

                            for (weapon = PISTOL_WEAPON; weapon < MAX_WEAPONS; weapon++)
                                g_player[myconnectindex].ps->gotweapon |= (1<<weapon);

                            for (weapon = PISTOL_WEAPON; weapon < MAX_WEAPONS; weapon++)
                                P_AddAmmo(g_player[myconnectindex].ps, weapon, g_player[myconnectindex].ps->max_ammo_amount[weapon]);

                            g_player[myconnectindex].ps->got_access = 7;
                        }
                    }
                    else
                    {
                        sprite[pi].extra = g_player[myconnectindex].ps->max_player_health;
                        actor[pi].extra = -1;
                        g_player[myconnectindex].ps->last_extra = g_player[myconnectindex].ps->max_player_health;
                        P_DoQuote(QUOTE_CHEAT_GODMODE_OFF, g_player[myconnectindex].ps);
                    }

                    sprite[pi].extra = g_player[myconnectindex].ps->max_player_health;
                    actor[pi].extra = 0;

                    if (k != CHEAT_COMEGETSOME)
                        g_player[myconnectindex].ps->dead_flag = 0;

                    end_cheat();
                    return;
                }

                case CHEAT_STUFF:
                    j = 0;

                    if (VOLUMEONE)
                        j = 6;

                    for (weapon = PISTOL_WEAPON; weapon < MAX_WEAPONS-j; weapon++)
                        g_player[myconnectindex].ps->gotweapon |= (1<<weapon);

                    for (weapon = PISTOL_WEAPON; weapon < MAX_WEAPONS-j; weapon++)
                        P_AddAmmo(g_player[myconnectindex].ps, weapon, g_player[myconnectindex].ps->max_ammo_amount[weapon]);

                    G_CheatGetInv();
                    g_player[myconnectindex].ps->got_access = 7;
                    P_DoQuote(QUOTE_CHEAT_EVERYTHING, g_player[myconnectindex].ps);

                    //                    P_DoQuote(QUOTE_21,g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->inven_icon = ICON_FIRSTAID;

                    end_cheat();
                    return;

                case CHEAT_SCOTTY:
                case CHEAT_SKILL:
                    if (k == CHEAT_SCOTTY)
                    {
                        i = Bstrlen(CheatStrings[k])-3+VOLUMEONE;
                        if (!consolecheat)
                        {
                            // JBF 20030914
                            int16_t volnume, levnume;
                            if (VOLUMEALL)
                            {
                                volnume = cheatbuf[i] - '0';
                                levnume = (cheatbuf[i+1] - '0')*10+(cheatbuf[i+2]-'0');
                            }
                            else
                            {
                                volnume =  cheatbuf[i] - '0';
                                levnume =  cheatbuf[i+1] - '0';
                            }

                            volnume--;
                            levnume--;

                            if ((VOLUMEONE && volnume > 0) || volnume > g_numVolumes-1 ||
                                levnume >= MAXLEVELS || aMapInfo[volnume *MAXLEVELS+levnume].filename == NULL)
                            {
                                end_cheat();
                                return;
                            }

                            ud.m_volume_number = ud.volume_number = volnume;
                            ud.m_level_number = ud.level_number = levnume;
                        }
                        else
                        {
                            // JBF 20030914
                            ud.m_volume_number = ud.volume_number = osdcmd_cheatsinfo_stat.volume;
                            ud.m_level_number = ud.level_number = osdcmd_cheatsinfo_stat.level;
                        }
                    }
                    else
                    {
                        if (!consolecheat)
                        {
                            i = Bstrlen(CheatStrings[k])-1;
                            ud.m_player_skill = ud.player_skill = cheatbuf[i] - '1';
                        }
                        else
                        {
                            ud.m_player_skill = ud.player_skill = osdcmd_cheatsinfo_stat.volume;
                        }
                    }
                    /*if (numplayers > 1 && g_netServer)
                    Net_NewGame(ud.m_volume_number,ud.m_level_number);
                    else*/ g_player[myconnectindex].ps->gm |= MODE_RESTART;

                    end_cheat();
                    return;

                case CHEAT_COORDS:
#ifdef USE_OPENGL
                    if (++ud.coords >= 3) ud.coords = 0;
#else
                    if (++ud.coords >= 2) ud.coords = 0;
#endif
                    end_cheat();
                    return;

                case CHEAT_VIEW:
                    if (g_player[myconnectindex].ps->over_shoulder_on)
                        g_player[myconnectindex].ps->over_shoulder_on = 0;
                    else
                    {
                        g_player[myconnectindex].ps->over_shoulder_on = 1;
                        CAMERADIST = 0;
                        CAMERACLOCK = totalclock;
                    }
                    //                    P_DoQuote(QUOTE_CHEATS_DISABLED,g_player[myconnectindex].ps);
                    end_cheat();
                    return;

                case CHEAT_TIME:
                    //                    P_DoQuote(QUOTE_21,g_player[myconnectindex].ps);
                    end_cheat();
                    return;

                case CHEAT_UNLOCK:
                    if (VOLUMEONE) return;

                    for (i=numsectors-1; i>=0; i--) //Unlock
                    {
                        j = sector[i].lotag;
                        if (j == -1 || j == 32767) continue;
                        if ((j & 0x7fff) > 2)
                        {
                            if (j&(0xffff-16384))
                                sector[i].lotag &= (0xffff-16384);
                            G_OperateSectors(i, g_player[myconnectindex].ps->i);
                        }
                    }
                    G_OperateForceFields(g_player[myconnectindex].ps->i, -1);

                    P_DoQuote(QUOTE_CHEAT_UNLOCK, g_player[myconnectindex].ps);
                    end_cheat();
                    return;

                case CHEAT_CASHMAN:
                    ud.cashman = 1-ud.cashman;
                    KB_ClearKeyDown(sc_N);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                    return;

                case CHEAT_ITEMS:
                    G_CheatGetInv();
                    g_player[myconnectindex].ps->got_access = 7;
                    P_DoQuote(QUOTE_CHEAT_EVERYTHING, g_player[myconnectindex].ps);
                    end_cheat();
                    return;

                case CHEAT_SHOWMAP: // SHOW ALL OF THE MAP TOGGLE;
                    ud.showallmap = !ud.showallmap;

                    for (i=0; i<(MAXSECTORS>>3); i++)
                        show2dsector[i] = ud.showallmap*255;

                    P_DoQuote(ud.showallmap ? QUOTE_SHOW_MAP_ON : QUOTE_SHOW_MAP_OFF,
                        g_player[myconnectindex].ps);

                    end_cheat();
                    return;

                case CHEAT_TODD:
                    if (NAM)
                    {
                        Bstrcpy(apStrings[QUOTE_RESERVED4], g_NAMMattCheatQuote);
                        P_DoQuote(QUOTE_RESERVED4, g_player[myconnectindex].ps);
                    }
                    else
                    {
                        P_DoQuote(QUOTE_CHEAT_TODD, g_player[myconnectindex].ps);
                    }

                    end_cheat();
                    return;

                case CHEAT_RATE:
                    if (ud.tickrate++ > 2)
                        ud.tickrate = 0;

                    end_cheat();
                    return;

                case CHEAT_BETA:
                    P_DoQuote(QUOTE_CHEAT_BETA, g_player[myconnectindex].ps);
                    KB_ClearKeyDown(sc_H);
                    end_cheat();
                    return;

                case CHEAT_HYPER:
                    g_player[myconnectindex].ps->inv_amount[GET_STEROIDS] = 399;
                    g_player[myconnectindex].ps->inv_amount[GET_HEATS] = 1200;
                    P_DoQuote(QUOTE_CHEAT_STEROIDS, g_player[myconnectindex].ps);
                    end_cheat();
                    return;

                case CHEAT_MONSTERS:
                {
                    const char *s [] ={ "On", "Off", "On (2)" };

                    if (++g_noEnemies == 3)
                        g_noEnemies = 0;

                    Bsprintf(apStrings[QUOTE_RESERVED4], "Monsters: %s", s[g_noEnemies]);
                    P_DoQuote(QUOTE_RESERVED4, g_player[myconnectindex].ps);

                    end_cheat();
                    return;
                }

                case CHEAT_RESERVED:
                case CHEAT_RESERVED3:
                    ud.eog = 1;
                    g_player[myconnectindex].ps->player_par = 0;
                    g_player[myconnectindex].ps->gm |= MODE_EOL;
                    KB_FlushKeyboardQueue();
                    return;

                default:
                    end_cheat();
                    return;
                }
            }
        }
    }
    else
    {
        if (KB_KeyPressed((uint8_t) CheatKeys[0]))
        {
            if (g_player[myconnectindex].ps->cheat_phase >= 0 && numplayers < 2 && ud.recstat == 0)
            {
                if (CheatKeys[0] == CheatKeys[1])
                    KB_ClearKeyDown((uint8_t) CheatKeys[0]);
                g_player[myconnectindex].ps->cheat_phase = -1;
            }
        }

        if (KB_KeyPressed((uint8_t) CheatKeys[1]))
        {
            if (g_player[myconnectindex].ps->cheat_phase == -1)
            {
                if (ud.player_skill == 4)
                {
                    P_DoQuote(QUOTE_CHEATS_DISABLED, g_player[myconnectindex].ps);
                    g_player[myconnectindex].ps->cheat_phase = 0;
                }
                else
                {
                    g_player[myconnectindex].ps->cheat_phase = 1;
                    //                    P_DoQuote(QUOTE_25,g_player[myconnectindex].ps);
                    cheatbuflen = 0;
                }
                KB_FlushKeyboardQueue();
            }
            else if (g_player[myconnectindex].ps->cheat_phase != 0)
            {
                g_player[myconnectindex].ps->cheat_phase = 0;
                KB_ClearKeyDown((uint8_t) CheatKeys[0]);
                KB_ClearKeyDown((uint8_t) CheatKeys[1]);
            }
        }
    }
}
