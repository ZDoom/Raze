//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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

#if 0

// Not yet implemented content kept for reference. Mostly multiplayer related.

const char *zMonsterStrings[] =
{
    "None",
    "Bring 'em on",
    "Respawn",
};

const char *zWeaponStrings[] =
{
    "Do not Respawn",
    "Are Permanent",
    "Respawn",
    "Respawn with Markers",
};

const char *zItemStrings[] =
{
    "Do not Respawn",
    "Respawn",
    "Respawn with Markers",
};

const char *zRespawnStrings[] =
{
    "At Random Locations",
    "Close to Weapons",
    "Away from Enemies",
};

const char *pzShowWeaponStrings[] = {
    "OFF",
    "SPRITE",
    "VOXEL"
};

CGameMenuItemTitle itemMessagesTitle("MESSAGES", 1, 160, 20, 2038);
CGameMenuItemZBool boolMessages("MESSAGES:", 3, 66, 70, 180, 0, SetMessages, NULL, NULL);
CGameMenuItemSlider sliderMsgCount("MESSAGE COUNT:", 3, 66, 80, 180, gMessageCount, 1, 16, 1, NULL, -1, -1);
CGameMenuItemSlider sliderMsgTime("MESSAGE TIME:", 3, 66, 90, 180, gMessageTime, 1, 8, 1, NULL, -1, -1);


CGameMenuItemTitle itemOptionsGameTitle("GAME SETUP", 1, 160, 20, 2038);


CGameMenuItemZCycle itemOptionsGameShowWeapons("SHOW WEAPONS:", 3, 66, 70, 180, 0, SetShowWeapons, pzShowWeaponStrings, ARRAY_SSIZE(pzShowWeaponStrings), 0);
CGameMenuItemZBool itemOptionsGameBoolSlopeTilting("SLOPE TILTING:", 3, 66, 80, 180, cl_slopetilting, SetSlopeTilting, NULL, NULL);
CGameMenuItemZBool itemOptionsGameBoolViewBobbing("VIEW BOBBING:", 3, 66, 90, 180, cl_viewvbob, SetViewBobbing, NULL, NULL);
CGameMenuItemZBool itemOptionsGameBoolViewSwaying("VIEW SWAYING:", 3, 66, 100, 180, cl_viewhbob, SetViewSwaying, NULL, NULL);

CGameMenuItemZBool itemOptionsDisplayBoolCenterHoriz("CENTER HORIZON LINE:", 3, 66, 90, 180, r_horizcenter, SetCenterHoriz, NULL, NULL);
CGameMenuItemZBool itemOptionsDisplayBoolPowerupDuration("POWERUP DURATION:", 3, 66, 110, 180, hud_powerupduration, SetPowerupDuration, NULL, NULL);
CGameMenuItemZBool itemOptionsDisplayBoolShowMapTitle("MAP TITLE:", 3, 66, 120, 180, hud_showmapname, SetShowMapTitle, NULL, NULL);


CGameMenuItemTitle itemNetworkHostTitle("HOST A GAME", 1, 160, 20, 2038);
CGameMenuItemSlider itemNetworkHostPlayerNum("PLAYER NUMBER:", 3, 66, 70, 180, 1, 2, kMaxPlayers, 1, NULL, -1, -1, kMenuSliderValue);
CGameMenuItemZEdit itemNetworkHostPort("NETWORK PORT:", 3, 66, 80, 180, zNetPortBuffer, 6, 0, NULL, 0);
CGameMenuItemChain itemNetworkHostHost("HOST A GAME", 3, 66, 100, 180, 1, NULL, -1, NetworkHostGame, 0);

CGameMenuItemTitle itemNetworkJoinTitle("JOIN A GAME", 1, 160, 20, 2038);
CGameMenuItemZEdit itemNetworkJoinAddress("NETWORK ADDRESS:", 3, 66, 70, 180, zNetAddressBuffer, 16, 0, NULL, 0);
CGameMenuItemZEdit itemNetworkJoinPort("NETWORK PORT:", 3, 66, 80, 180, zNetPortBuffer, 6, 0, NULL, 0);
CGameMenuItemChain itemNetworkJoinJoin("JOIN A GAME", 3, 66, 100, 180, 1, NULL, -1, NetworkJoinGame, 0);


////
void SetWeaponsV10X(CGameMenuItemZBool* pItem)
{
    if (gGameOptions.nGameType == 0) {
        gWeaponsV10x = pItem->at20;
        gGameOptions.weaponsV10x = pItem->at20;
    }
}
////

void SetTurnSpeed(CGameMenuItemSlider *pItem)
{
    gTurnSpeed = pItem->nValue;
}

void SetAutoAim(CGameMenuItemZCycle *pItem)
{
    cl_autoaim = pItem->m_nFocus;
    if (!gDemo.at0 && !gDemo.at1)
    {
        gProfile[myconnectindex].nAutoAim = cl_autoaim;
        netBroadcastPlayerInfo(myconnectindex);
    }
}

void SetupNetworkMenu(void)
{
    sprintf(zNetPortBuffer, "%d", gNetPort);
    if (strlen(gNetAddress) > 0)
        strncpy(zNetAddressBuffer, gNetAddress, sizeof(zNetAddressBuffer)-1);

    menuNetwork.Add(&itemNetworkTitle, false);
    menuNetwork.Add(&itemNetworkHost, true);
    menuNetwork.Add(&itemNetworkJoin, false);
    menuNetwork.Add(&itemBloodQAV, false);

    menuNetworkHost.Add(&itemNetworkHostTitle, false);
    menuNetworkHost.Add(&itemNetworkHostPlayerNum, true);
    menuNetworkHost.Add(&itemNetworkHostPort, false);
    menuNetworkHost.Add(&itemNetworkHostHost, false);
    menuNetworkHost.Add(&itemBloodQAV, false);

    menuNetworkJoin.Add(&itemNetworkJoinTitle, false);
    menuNetworkJoin.Add(&itemNetworkJoinAddress, true);
    menuNetworkJoin.Add(&itemNetworkJoinPort, false);
    menuNetworkJoin.Add(&itemNetworkJoinJoin, false);
    menuNetworkJoin.Add(&itemBloodQAV, false);
}

void SetupNetworkHostMenu(CGameMenuItemChain *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
}

void SetupNetworkJoinMenu(CGameMenuItemChain *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
}

void NetworkHostGame(CGameMenuItemChain *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
    Mus_Stop();
    FX_StopAllSounds();
    UpdateDacs(0, true);
    gNetPlayers = itemNetworkHostPlayerNum.nValue;
    gNetPort = strtoul(zNetPortBuffer, NULL, 10);
    if (!gNetPort)
        gNetPort = kNetDefaultPort;
    gNetMode = NETWORK_SERVER;
    netInitialize(false);
    gGameMenuMgr.Deactivate();
    gQuitGame = gRestartGame = true;
}

void NetworkJoinGame(CGameMenuItemChain *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
    Mus_Stop();
    FX_StopAllSounds();
    UpdateDacs(0, true);
    strcpy(gNetAddress, zNetAddressBuffer);
    gNetPort = strtoul(zNetPortBuffer, NULL, 10);
    if (!gNetPort)
        gNetPort = kNetDefaultPort;
    gNetMode = NETWORK_CLIENT;
    netInitialize(false);
    gGameMenuMgr.Deactivate();
    gQuitGame = gRestartGame = true;
}


void Restart(CGameMenuItemChain *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
    if (gGameOptions.nGameType == 0 || numplayers == 1)
    {
        gQuitGame = true;
        gRestartGame = true;
    }
    else
        gQuitRequest = 2;
    gGameMenuMgr.Deactivate();
}



#endif

