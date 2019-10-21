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
#include "ns.h"	// Must come before everything else!

#include "compat.h"
#include "mmulti.h"
#include "common_game.h"
#include "fx_man.h"
#include "music.h"
#include "blood.h"
#include "demo.h"
#include "config.h"
#include "gamemenu.h"
#include "globals.h"
#include "loadsave.h"
#include "menu.h"
#include "messages.h"
#include "network.h"
#include "osdcmds.h"
#include "sfx.h"
#include "screen.h"
#include "sound.h"
#include "view.h"

BEGIN_BLD_NS

void SaveGame(CGameMenuItemZEditBitmap *, CGameMenuEvent *);

void SaveGameProcess(CGameMenuItemChain *);
void SetDifficultyAndStart(CGameMenuItemChain *);
void SetDetail(CGameMenuItemSlider *);
void SetGamma(CGameMenuItemSlider *);
void SetMusicVol(CGameMenuItemSlider *);
void SetSoundVol(CGameMenuItemSlider *);
void SetCDVol(CGameMenuItemSlider *);
void SetDoppler(CGameMenuItemZBool *);
void SetCrosshair(CGameMenuItemZBool *);
void SetCenterHoriz(CGameMenuItemZBool *);
void SetShowWeapons(CGameMenuItemZBool *);
void SetSlopeTilting(CGameMenuItemZBool *);
void SetViewBobbing(CGameMenuItemZBool *);
void SetViewSwaying(CGameMenuItemZBool *);
void SetMouseSensitivity(CGameMenuItemSliderFloat *);
void SetMouseAimFlipped(CGameMenuItemZBool *);
void SetTurnSpeed(CGameMenuItemSlider *);
void ResetKeys(CGameMenuItemChain *);
void ResetKeysClassic(CGameMenuItemChain *);
void SetMessages(CGameMenuItemZBool *);
void LoadGame(CGameMenuItemZEditBitmap *, CGameMenuEvent *);
void SetupNetLevels(CGameMenuItemZCycle *);
void StartNetGame(CGameMenuItemChain *);
void SetParentalLock(CGameMenuItemZBool *);
void TenProcess(CGameMenuItem7EA1C *);
void SetupLevelMenuItem(int);
void SetupVideoModeMenu(CGameMenuItemChain *);
void SetVideoMode(CGameMenuItemChain *);
void SetWidescreen(CGameMenuItemZBool *);
void SetFOV(CGameMenuItemSlider *);
void UpdateVideoModeMenuFrameLimit(CGameMenuItemZCycle *pItem);
void UpdateVideoModeMenuFPSOffset(CGameMenuItemSlider *pItem);
void UpdateVideoColorMenu(CGameMenuItemSliderFloat *);
void ResetVideoColor(CGameMenuItemChain *);
void SetWeaponsV10X(CGameMenuItemZBool* pItem);

#ifdef USE_OPENGL
void SetupVideoPolymostMenu(CGameMenuItemChain *);
#endif

char strRestoreGameStrings[][16] = 
{
    "<Empty>",
    "<Empty>",
    "<Empty>",
    "<Empty>",
    "<Empty>",
    "<Empty>",
    "<Empty>",
    "<Empty>",
    "<Empty>",
    "<Empty>",
};

const char *zNetGameTypes[] =
{
    "Cooperative",
    "Bloodbath",
    "Teams",
};

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

const char *zDiffStrings[] =
{
    "STILL KICKING",
    "PINK ON THE INSIDE",
    "LIGHTLY BROILED",
    "WELL DONE",
    "EXTRA CRISPY",
};

char zUserMapName[16];
const char *zEpisodeNames[6];
const char *zLevelNames[6][16];

static char MenuGameFuncs[NUMGAMEFUNCTIONS][MAXGAMEFUNCLEN];
static char const *MenuGameFuncNone = "  -None-";
static char const *pzGamefuncsStrings[NUMGAMEFUNCTIONS + 1];
static int nGamefuncsValues[NUMGAMEFUNCTIONS + 1];
static int nGamefuncsNum;

CGameMenu menuMain;
CGameMenu menuMainWithSave;
CGameMenu menuNetMain;
CGameMenu menuNetStart;
CGameMenu menuEpisode;
CGameMenu menuDifficulty;
CGameMenu menuOptionsOld;
CGameMenu menuControls;
CGameMenu menuMessages;
CGameMenu menuKeys;
CGameMenu menuSaveGame;
CGameMenu menuLoadGame;
CGameMenu menuLoading;
CGameMenu menuSounds;
CGameMenu menuQuit;
CGameMenu menuRestart;
CGameMenu menuCredits;
CGameMenu menuOrder;
CGameMenu menuPlayOnline;
CGameMenu menuParentalLock;
CGameMenu menuSorry;
CGameMenu menuSorry2;
CGameMenu menuNetwork;
CGameMenu menuNetworkHost;
CGameMenu menuNetworkJoin;

CGameMenuItemQAV itemBloodQAV("", 3, 160, 100, "BDRIP", true);
CGameMenuItemQAV itemCreditsQAV("", 3, 160, 100, "CREDITS", false, true);
CGameMenuItemQAV itemHelp3QAV("", 3, 160, 100, "HELP3", false, false);
CGameMenuItemQAV itemHelp3BQAV("", 3, 160, 100, "HELP3B", false, false);
CGameMenuItemQAV itemHelp4QAV("", 3, 160, 100, "HELP4", false, true);
CGameMenuItemQAV itemHelp5QAV("", 3, 160, 100, "HELP5", false, true);

CGameMenuItemTitle itemMainTitle("BLOOD", 1, 160, 20, 2038);
CGameMenuItemChain itemMain1("NEW GAME", 1, 0, 45, 320, 1, &menuEpisode, -1, NULL, 0);
//CGameMenuItemChain itemMain2("PLAY ONLINE", 1, 0, 65, 320, 1, &menuPlayOnline, -1, NULL, 0);
CGameMenuItemChain itemMain2("MULTIPLAYER", 1, 0, 65, 320, 1, &menuNetwork, -1, NULL, 0);
CGameMenuItemChain itemMain3("OPTIONS", 1, 0, 85, 320, 1, &menuOptions, -1, NULL, 0);
CGameMenuItemChain itemMain4("LOAD GAME", 1, 0, 105, 320, 1, &menuLoadGame, -1, NULL, 0);
CGameMenuItemChain itemMain5("HELP", 1, 0, 125, 320, 1, &menuOrder, -1, NULL, 0);
CGameMenuItemChain itemMain6("CREDITS", 1, 0, 145, 320, 1, &menuCredits, -1, NULL, 0);
CGameMenuItemChain itemMain7("QUIT", 1, 0, 165, 320, 1, &menuQuit, -1, NULL, 0);

CGameMenuItemTitle itemMainSaveTitle("BLOOD", 1, 160, 20, 2038);
CGameMenuItemChain itemMainSave1("NEW GAME", 1, 0, 45, 320, 1, &menuEpisode, -1, NULL, 0);
//CGameMenuItemChain itemMainSave2("PLAY ONLINE", 1, 0, 60, 320, 1, &menuPlayOnline, -1, NULL, 0);
CGameMenuItemChain itemMainSave2("OPTIONS", 1, 0, 60, 320, 1, &menuOptions, -1, NULL, 0);
CGameMenuItemChain itemMainSave3("SAVE GAME", 1, 0, 75, 320, 1, &menuSaveGame, -1, SaveGameProcess, 0);
CGameMenuItemChain itemMainSave4("LOAD GAME", 1, 0, 90, 320, 1, &menuLoadGame, -1, NULL, 0);
CGameMenuItemChain itemMainSave5("HELP", 1, 0, 105, 320, 1, &menuOrder, -1, NULL, 0);
CGameMenuItemChain itemMainSave6("CREDITS", 1, 0, 120, 320, 1, &menuCredits, -1, NULL, 0);
CGameMenuItemChain itemMainSave7("END GAME", 1, 0, 135, 320, 1, &menuRestart, -1, NULL, 0);
CGameMenuItemChain itemMainSave8("QUIT", 1, 0, 150, 320, 1, &menuQuit, -1, NULL, 0);

CGameMenuItemTitle itemEpisodesTitle("EPISODES", 1, 160, 20, 2038);
CGameMenuItemChain7F2F0 itemEpisodes[6];

CGameMenuItemTitle itemDifficultyTitle("DIFFICULTY", 1, 160, 20, 2038);
CGameMenuItemChain itemDifficulty1("STILL KICKING", 1, 0, 60, 320, 1, NULL, -1, SetDifficultyAndStart, 0);
CGameMenuItemChain itemDifficulty2("PINK ON THE INSIDE", 1, 0, 80, 320, 1, NULL, -1, SetDifficultyAndStart, 1);
CGameMenuItemChain itemDifficulty3("LIGHTLY BROILED", 1, 0, 100, 320, 1, NULL, -1, SetDifficultyAndStart, 2);
CGameMenuItemChain itemDifficulty4("WELL DONE", 1, 0, 120, 320, 1, NULL, -1, SetDifficultyAndStart, 3);
CGameMenuItemChain itemDifficulty5("EXTRA CRISPY", 1, 0, 140, 320, 1, 0, -1, SetDifficultyAndStart, 4);

CGameMenuItemTitle itemOptionsOldTitle("OPTIONS", 1, 160, 20, 2038);
CGameMenuItemChain itemOption1("CONTROLS...", 3, 0, 40, 320, 1, &menuControls, -1, NULL, 0);
CGameMenuItemSlider sliderDetail("DETAIL:", 3, 66, 50, 180, gDetail, 0, 4, 1, SetDetail, -1, -1);
CGameMenuItemSlider sliderGamma("GAMMA:", 3, 66, 60, 180, gGamma, 0, 15, 2, SetGamma, -1, -1);
CGameMenuItemSlider sliderMusic("MUSIC:", 3, 66, 70, 180, MusicVolume, 0, 256, 48, SetMusicVol, -1, -1);
CGameMenuItemSlider sliderSound("SOUND:", 3, 66, 80, 180, FXVolume, 0, 256, 48, SetSoundVol, -1, -1);
CGameMenuItemSlider sliderCDAudio("CD AUDIO:", 3, 66, 90, 180, CDVolume, 0, 256, 48, SetCDVol, -1, -1);
CGameMenuItemZBool bool3DAudio("3D AUDIO:", 3, 66, 100, 180, gDoppler, SetDoppler, NULL, NULL);
CGameMenuItemZBool boolCrosshair("CROSSHAIR:", 3, 66, 110, 180, cl_crosshair, SetCrosshair, NULL, NULL);
CGameMenuItemZBool boolShowWeapons("SHOW WEAPONS:", 3, 66, 120, 180, gShowWeapon, SetShowWeapons, NULL, NULL);
CGameMenuItemZBool boolSlopeTilting("SLOPE TILTING:", 3, 66, 130, 180, gSlopeTilting, SetSlopeTilting, NULL, NULL);
CGameMenuItemZBool boolViewBobbing("VIEW BOBBING:", 3, 66, 140, 180, gViewVBobbing, SetViewBobbing, NULL, NULL);
CGameMenuItemZBool boolViewSwaying("VIEW SWAYING:", 3, 66, 150, 180, gViewHBobbing, SetViewSwaying, NULL, NULL);
CGameMenuItem7EE34 itemOption2("VIDEO MODE...", 3, 0, 160, 320, 1);
CGameMenuItemChain itemChainParentalLock("PARENTAL LOCK", 3, 0, 170, 320, 1, &menuParentalLock, -1, NULL, 0);

CGameMenuItemTitle itemControlsTitle("CONTROLS", 1, 160, 20, 2038);
CGameMenuItemSliderFloat sliderMouseSpeed("Mouse Sensitivity:", 1, 10, 70, 300, CONTROL_MouseSensitivity, 0.5f, 16.f, 0.5f, SetMouseSensitivity, -1,-1);
CGameMenuItemZBool boolMouseFlipped("Invert Mouse Aim:", 1, 10, 90, 300, gMouseAimingFlipped, SetMouseAimFlipped, NULL, NULL);
CGameMenuItemSlider sliderTurnSpeed("Key Turn Speed:", 1, 10, 110, 300, gTurnSpeed, 64, 128, 4, SetTurnSpeed, -1, -1);
CGameMenuItemChain itemChainKeyList("Configure Keys...", 1, 0, 130, 320, 1, &menuKeys, -1, NULL, 0);
CGameMenuItemChain itemChainKeyReset("Reset Keys (default)...", 1, 0, 150, 320, 1, &menuKeys, -1, ResetKeys, 0);
CGameMenuItemChain itemChainKeyResetClassic("Reset Keys (classic)...", 1, 0, 170, 320, 1, &menuKeys, -1, ResetKeysClassic, 0);

CGameMenuItemTitle itemMessagesTitle("MESSAGES", 1, 160, 20, 2038);
CGameMenuItemZBool boolMessages("MESSAGES:", 3, 66, 70, 180, 0, SetMessages, NULL, NULL);
CGameMenuItemSlider sliderMsgCount("MESSAGE COUNT:", 3, 66, 80, 180, gMessageCount, 1, 16, 1, NULL, -1, -1);
CGameMenuItemSlider sliderMsgTime("MESSAGE TIME:", 3, 66, 90, 180, gMessageTime, 1, 8, 1, NULL, -1, -1);
CGameMenuItemZBool boolMsgFont("LARGE FONT:", 3, 66, 100, 180, 0, 0, NULL, NULL);
CGameMenuItemZBool boolMsgIncoming("INCOMING:", 3, 66, 110, 180, 0, 0, NULL, NULL);
CGameMenuItemZBool boolMsgSelf("SELF PICKUP:", 3, 66, 120, 180, 0, 0, NULL, NULL);
CGameMenuItemZBool boolMsgOther("OTHER PICKUP:", 3, 66, 130, 180, 0, 0, NULL, NULL);
CGameMenuItemZBool boolMsgRespawn("RESPAWN:", 3, 66, 140, 180, 0, 0, NULL, NULL);

CGameMenuItemTitle itemKeysTitle("KEY SETUP", 1, 160, 20, 2038);
CGameMenuItemKeyList itemKeyList("", 3, 56, 40, 200, 16, NUMGAMEFUNCTIONS, 0);

CGameMenuItemTitle itemSaveTitle("Save Game", 1, 160, 20, 2038);
CGameMenuItemZEditBitmap itemSaveGame1(NULL, 3, 20, 60, 320, strRestoreGameStrings[0], 16, 1, SaveGame, 0);
CGameMenuItemZEditBitmap itemSaveGame2(NULL, 3, 20, 70, 320, strRestoreGameStrings[1], 16, 1, SaveGame, 1);
CGameMenuItemZEditBitmap itemSaveGame3(NULL, 3, 20, 80, 320, strRestoreGameStrings[2], 16, 1, SaveGame, 2);
CGameMenuItemZEditBitmap itemSaveGame4(NULL, 3, 20, 90, 320, strRestoreGameStrings[3], 16, 1, SaveGame, 3);
CGameMenuItemZEditBitmap itemSaveGame5(NULL, 3, 20, 100, 320, strRestoreGameStrings[4], 16, 1, SaveGame, 4);
CGameMenuItemZEditBitmap itemSaveGame6(NULL, 3, 20, 110, 320, strRestoreGameStrings[5], 16, 1, SaveGame, 5);
CGameMenuItemZEditBitmap itemSaveGame7(NULL, 3, 20, 120, 320, strRestoreGameStrings[6], 16, 1, SaveGame, 6);
CGameMenuItemZEditBitmap itemSaveGame8(NULL, 3, 20, 130, 320, strRestoreGameStrings[7], 16, 1, SaveGame, 7);
CGameMenuItemZEditBitmap itemSaveGame9(NULL, 3, 20, 140, 320, strRestoreGameStrings[8], 16, 1, SaveGame, 8);
CGameMenuItemZEditBitmap itemSaveGame10(NULL, 3, 20, 150, 320, strRestoreGameStrings[9], 16, 1, SaveGame, 9);
CGameMenuItemBitmapLS itemSaveGamePic(NULL, 3, 0, 0, 2050);

CGameMenuItemTitle itemLoadTitle("Load Game", 1, 160, 20, 2038);
CGameMenuItemZEditBitmap itemLoadGame1(NULL, 3, 20, 60, 320, strRestoreGameStrings[0], 16, 1, LoadGame, 0);
CGameMenuItemZEditBitmap itemLoadGame2(NULL, 3, 20, 70, 320, strRestoreGameStrings[1], 16, 1, LoadGame, 1);
CGameMenuItemZEditBitmap itemLoadGame3(NULL, 3, 20, 80, 320, strRestoreGameStrings[2], 16, 1, LoadGame, 2);
CGameMenuItemZEditBitmap itemLoadGame4(NULL, 3, 20, 90, 320, strRestoreGameStrings[3], 16, 1, LoadGame, 3);
CGameMenuItemZEditBitmap itemLoadGame5(NULL, 3, 20, 100, 320, strRestoreGameStrings[4], 16, 1, LoadGame, 4);
CGameMenuItemZEditBitmap itemLoadGame6(NULL, 3, 20, 110, 320, strRestoreGameStrings[5], 16, 1, LoadGame, 5);
CGameMenuItemZEditBitmap itemLoadGame7(NULL, 3, 20, 120, 320, strRestoreGameStrings[6], 16, 1, LoadGame, 6);
CGameMenuItemZEditBitmap itemLoadGame8(NULL, 3, 20, 130, 320, strRestoreGameStrings[7], 16, 1, LoadGame, 7);
CGameMenuItemZEditBitmap itemLoadGame9(NULL, 3, 20, 140, 320, strRestoreGameStrings[8], 16, 1, LoadGame, 8);
CGameMenuItemZEditBitmap itemLoadGame10(NULL, 3, 20, 150, 320, strRestoreGameStrings[9], 16, 1, LoadGame, 9);
CGameMenuItemBitmapLS itemLoadGamePic(NULL, 3, 0, 0, 2518);

CGameMenuItemTitle itemNetStartTitle("MULTIPLAYER", 1, 160, 20, 2038);
CGameMenuItemZCycle itemNetStart1("GAME:", 3, 66, 60, 180, 0, 0, zNetGameTypes, 3, 0);
CGameMenuItemZCycle itemNetStart2("EPISODE:", 3, 66, 70, 180, 0, SetupNetLevels, NULL, 0, 0);
CGameMenuItemZCycle itemNetStart3("LEVEL:", 3, 66, 80, 180, 0, NULL, NULL, 0, 0);
CGameMenuItemZCycle itemNetStart4("DIFFICULTY:", 3, 66, 90, 180, 0, 0, zDiffStrings, 5, 0);
CGameMenuItemZCycle itemNetStart5("MONSTERS:", 3, 66, 100, 180, 0, 0, zMonsterStrings, 3, 0);
CGameMenuItemZCycle itemNetStart6("WEAPONS:", 3, 66, 110, 180, 0, 0, zWeaponStrings, 4, 0);
CGameMenuItemZCycle itemNetStart7("ITEMS:", 3, 66, 120, 180, 0, 0, zItemStrings, 3, 0);
CGameMenuItemZBool itemNetStart8("FRIENDLY FIRE:", 3, 66, 130, 180, true, 0, NULL, NULL);
CGameMenuItemZBool itemNetStart9("KEEP KEYS ON RESPAWN:", 3, 66, 140, 180, false, 0, NULL, NULL);
CGameMenuItemZBool itemNetStart10("V1.0x WEAPONS BALANCE:", 3, 66, 150, 180, false, 0, NULL, NULL);
CGameMenuItemZEdit itemNetStart11("USER MAP:", 3, 66, 160, 180, zUserMapName, 13, 0, NULL, 0);
CGameMenuItemChain itemNetStart12("START GAME", 1, 66, 175, 280, 0, 0, -1, StartNetGame, 0);

CGameMenuItemText itemLoadingText("LOADING...", 1, 160, 100, 1);

CGameMenuItemTitle itemSoundsTitle("SOUNDS", 1, 160, 20, 2038);
CGameMenuItemSlider itemSoundsMusic("MUSIC:", 3, 40, 60, 180, MusicVolume, 0, 256, 48, SetMusicVol, -1, -1);
CGameMenuItemSlider itemSoundsSound("SOUND:", 3, 40, 70, 180, FXVolume, 0, 256, 48, SetSoundVol, -1, -1);
CGameMenuItemSlider itemSoundsCDAudio("CD AUDIO:", 3, 40, 80, 180, CDVolume, 0, 256, 48, SetCDVol, -1, -1);
CGameMenuItemZBool itemSounds3DAudio("3D SOUND:", 3, 40, 90, 180, gDoppler, SetDoppler, NULL, NULL);

CGameMenuItemTitle itemQuitTitle("QUIT", 1, 160, 20, 2038);
CGameMenuItemText itemQuitText1("Do you really want to quit?", 0, 160, 100, 1);
CGameMenuItemYesNoQuit itemQuitYesNo("[Y/N]", 0, 20, 110, 280, 1, 0);

CGameMenuItemTitle itemRestartTitle("RESTART GAME", 1, 160, 20, 2038);
CGameMenuItemText itemRestartText1("Do you really want to restart game?", 0, 160, 100, 1);
CGameMenuItemYesNoQuit itemRestartYesNo("[Y/N]", 0, 20, 110, 280, 1, 1);

CGameMenuItemPicCycle itemCreditsPicCycle(0, 0, NULL, NULL, 0, 0);
CGameMenuItemPicCycle itemOrderPicCycle(0, 0, NULL, NULL, 0, 0);

CGameMenuItemTitle itemParentalLockTitle("PARENTAL LOCK", 1, 160, 20, 2038);
CGameMenuItemZBool itemParentalLockToggle("LOCK:", 3, 66, 70, 180, 0, SetParentalLock, NULL, NULL);
CGameMenuItemPassword itemParentalLockPassword("SET PASSWORD:", 3, 160, 80);

CGameMenuItemPicCycle itemSorryPicCycle(0, 0, NULL, NULL, 0, 0);
CGameMenuItemText itemSorryText1("Loading and saving games", 0, 160, 90, 1);
CGameMenuItemText itemSorryText2("not supported", 0, 160, 100, 1);
CGameMenuItemText itemSorryText3("in this demo version of Blood.", 0, 160, 110, 1);

CGameMenuItemText itemSorry2Text1("Buy the complete version of", 0, 160, 90, 1);
CGameMenuItemText itemSorry2Text2("Blood for three new episodes", 0, 160, 100, 1);
CGameMenuItemText itemSorry2Text3("plus eight BloodBath-only levels!", 0, 160, 110, 1);

CGameMenuItemTitle unk_26E06C(" ONLINE ", 1, 160, 20, 2038);
CGameMenuItem7EA1C unk_26E090("DWANGO", 1, 0, 45, 320, "matt", "DWANGO", 1, -1, NULL, 0);
CGameMenuItem7EA1C unk_26E0E8("RTIME", 1, 0, 65, 320, "matt", "RTIME", 1, -1, NULL, 0);
CGameMenuItem7EA1C unk_26E140("HEAT", 1, 0, 85, 320, "matt", "HEAT", 1, -1, NULL, 0);
CGameMenuItem7EA1C unk_26E198("KALI", 1, 0, 105, 320, "matt", "KALI", 1, -1, NULL, 0);
CGameMenuItem7EA1C unk_26E1F0("MPATH", 1, 0, 125, 320, "matt", "MPATH", 1, -1, NULL, 0);
CGameMenuItem7EA1C unk_26E248("TEN", 1, 0, 145, 320, "matt", "TEN", 1, -1, TenProcess, 0);


// static int32_t newresolution, newrendermode, newfullscreen, newvsync;

enum resflags_t {
    RES_FS = 0x1,
    RES_WIN = 0x2,
};

#define MAXRESOLUTIONSTRINGLENGTH 19

struct resolution_t {
    int32_t xdim, ydim;
    int32_t flags;
    int32_t bppmax;
    char name[MAXRESOLUTIONSTRINGLENGTH];
};

resolution_t gResolution[MAXVALIDMODES];
int gResolutionNum;
const char *gResolutionName[MAXVALIDMODES];

CGameMenu menuOptions;
CGameMenu menuOptionsGame;
CGameMenu menuOptionsDisplay;
CGameMenu menuOptionsDisplayColor;
CGameMenu menuOptionsDisplayMode;
#ifdef USE_OPENGL
CGameMenu menuOptionsDisplayPolymost;
#endif
CGameMenu menuOptionsSound;
CGameMenu menuOptionsPlayer;
CGameMenu menuOptionsControl;

void SetupOptionsSound(CGameMenuItemChain *pItem);

CGameMenuItemTitle itemOptionsTitle("OPTIONS", 1, 160, 20, 2038);
CGameMenuItemChain itemOptionsChainGame("GAME SETUP", 1, 0, 50, 320, 1, &menuOptionsGame, -1, NULL, 0);
CGameMenuItemChain itemOptionsChainDisplay("DISPLAY SETUP", 1, 0, 70, 320, 1, &menuOptionsDisplay, -1, NULL, 0);
CGameMenuItemChain itemOptionsChainSound("SOUND SETUP", 1, 0, 90, 320, 1, &menuOptionsSound, -1, SetupOptionsSound, 0);
CGameMenuItemChain itemOptionsChainPlayer("PLAYER SETUP", 1, 0, 110, 320, 1, &menuOptionsPlayer, -1, NULL, 0);
CGameMenuItemChain itemOptionsChainControl("CONTROL SETUP", 1, 0, 130, 320, 1, &menuOptionsControl, -1, NULL, 0);
CGameMenuItemChain itemOptionsChainOld("OLD MENU", 1, 0, 170, 320, 1, &menuOptionsOld, -1, NULL, 0);

const char *pzAutoAimStrings[] = {
    "NEVER",
    "ALWAYS",
    "HITSCAN ONLY"
};

const char *pzWeaponSwitchStrings[] = {
    "NEVER",
    "IF NEW",
    "BY RATING"
};

void SetAutoAim(CGameMenuItemZCycle *);
void SetLevelStats(CGameMenuItemZBool *);
void SetPowerupDuration(CGameMenuItemZBool *);
void SetShowMapTitle(CGameMenuItemZBool*);
void SetWeaponSwitch(CGameMenuItemZCycle *pItem);

CGameMenuItemTitle itemOptionsGameTitle("GAME SETUP", 1, 160, 20, 2038);

///////////////
CGameMenuItemZBool itemOptionsGameBoolWeaponsV10X("V1.0x WEAPONS BALANCE:", 3, 66, 130, 180, gWeaponsV10x, SetWeaponsV10X, NULL, NULL);
///////////////////

CGameMenuItemZBool itemOptionsGameBoolShowWeapons("SHOW WEAPONS:", 3, 66, 70, 180, gShowWeapon, SetShowWeapons, NULL, NULL);
CGameMenuItemZBool itemOptionsGameBoolSlopeTilting("SLOPE TILTING:", 3, 66, 80, 180, gSlopeTilting, SetSlopeTilting, NULL, NULL);
CGameMenuItemZBool itemOptionsGameBoolViewBobbing("VIEW BOBBING:", 3, 66, 90, 180, gViewVBobbing, SetViewBobbing, NULL, NULL);
CGameMenuItemZBool itemOptionsGameBoolViewSwaying("VIEW SWAYING:", 3, 66, 100, 180, gViewHBobbing, SetViewSwaying, NULL, NULL);
CGameMenuItemZCycle itemOptionsGameBoolAutoAim("AUTO AIM:", 3, 66, 110, 180, 0, SetAutoAim, pzAutoAimStrings, ARRAY_SSIZE(pzAutoAimStrings), 0);
CGameMenuItemZCycle itemOptionsGameWeaponSwitch("EQUIP PICKUPS:", 3, 66, 120, 180, 0, SetWeaponSwitch, pzWeaponSwitchStrings, ARRAY_SSIZE(pzWeaponSwitchStrings), 0);
CGameMenuItemChain itemOptionsGameChainParentalLock("PARENTAL LOCK", 3, 0, 120, 320, 1, &menuParentalLock, -1, NULL, 0);

CGameMenuItemTitle itemOptionsDisplayTitle("DISPLAY SETUP", 1, 160, 20, 2038);
CGameMenuItemChain itemOptionsDisplayColor("COLOR CORRECTION", 3, 66, 60, 180, 0, &menuOptionsDisplayColor, -1, NULL, 0);
CGameMenuItemChain itemOptionsDisplayMode("VIDEO MODE", 3, 66, 70, 180, 0, &menuOptionsDisplayMode, -1, SetupVideoModeMenu, 0);
CGameMenuItemZBool itemOptionsDisplayBoolCrosshair("CROSSHAIR:", 3, 66, 80, 180, cl_crosshair, SetCrosshair, NULL, NULL);
CGameMenuItemZBool itemOptionsDisplayBoolCenterHoriz("CENTER HORIZON LINE:", 3, 66, 90, 180, gCenterHoriz, SetCenterHoriz, NULL, NULL);
CGameMenuItemZBool itemOptionsDisplayBoolLevelStats("LEVEL STATS:", 3, 66, 100, 180, gLevelStats, SetLevelStats, NULL, NULL);
CGameMenuItemZBool itemOptionsDisplayBoolPowerupDuration("POWERUP DURATION:", 3, 66, 110, 180, gPowerupDuration, SetPowerupDuration, NULL, NULL);
CGameMenuItemZBool itemOptionsDisplayBoolShowMapTitle("MAP TITLE:", 3, 66, 120, 180, gShowMapTitle, SetShowMapTitle, NULL, NULL);
CGameMenuItemZBool itemOptionsDisplayBoolMessages("MESSAGES:", 3, 66, 130, 180, gMessageState, SetMessages, NULL, NULL);
CGameMenuItemZBool itemOptionsDisplayBoolWidescreen("WIDESCREEN:", 3, 66, 140, 180, r_usenewaspect, SetWidescreen, NULL, NULL);
CGameMenuItemSlider itemOptionsDisplayFOV("FOV:", 3, 66, 150, 180, &gFov, 75, 140, 5, SetFOV, -1, -1, kMenuSliderValue);
#ifdef USE_OPENGL
CGameMenuItemChain itemOptionsDisplayPolymost("POLYMOST SETUP", 3, 66, 160, 180, 0, &menuOptionsDisplayPolymost, -1, SetupVideoPolymostMenu, 0);
#endif

const char *pzRendererStrings[] = {
    "CLASSIC",
    "POLYMOST"
};

const int nRendererValues[] = {
    REND_CLASSIC,
    REND_POLYMOST
};

const char *pzVSyncStrings[] = {
    "ADAPTIVE",
    "OFF",
    "ON"
};

const int nVSyncValues[] = {
    -1,
    0,
    1
};

const char *pzFrameLimitStrings[] = {
    "30 FPS",
    "60 FPS",
    "75 FPS",
    "100 FPS",
    "120 FPS",
    "144 FPS",
    "165 FPS",
    "240 FPS"
};

const int nFrameLimitValues[] = {
    30,
    60,
    75,
    100,
    120,
    144,
    165,
    240
};


void PreDrawVideoModeMenu(CGameMenuItem *);

CGameMenuItemTitle itemOptionsDisplayModeTitle("VIDEO MODE", 1, 160, 20, 2038);
CGameMenuItemZCycle itemOptionsDisplayModeResolution("RESOLUTION:", 3, 66, 60, 180, 0, NULL, NULL, 0, 0, true);
CGameMenuItemZCycle itemOptionsDisplayModeRenderer("RENDERER:", 3, 66, 70, 180, 0, NULL, pzRendererStrings, 2, 0);
CGameMenuItemZBool itemOptionsDisplayModeFullscreen("FULLSCREEN:", 3, 66, 80, 180, 0, NULL, NULL, NULL);
CGameMenuItemZCycle itemOptionsDisplayModeVSync("VSYNC:", 3, 66, 90, 180, 0, NULL, pzVSyncStrings, 3, 0);
CGameMenuItemZCycle itemOptionsDisplayModeFrameLimit("FRAMERATE LIMIT:", 3, 66, 100, 180, 0, UpdateVideoModeMenuFrameLimit, pzFrameLimitStrings, 8, 0);
CGameMenuItemSlider itemOptionsDisplayModeFPSOffset("FPS OFFSET:", 3, 66, 110, 180, 0, -10, 10, 1, UpdateVideoModeMenuFPSOffset, -1, -1, kMenuSliderValue);
CGameMenuItemChain itemOptionsDisplayModeApply("APPLY CHANGES", 3, 66, 125, 180, 0, NULL, 0, SetVideoMode, 0);

void PreDrawDisplayColor(CGameMenuItem *);

CGameMenuItemTitle itemOptionsDisplayColorTitle("COLOR CORRECTION", 1, 160, 20, -1);
CGameMenuItemSliderFloat itemOptionsDisplayColorGamma("GAMMA:", 3, 66, 140, 180, &g_videoGamma, 0.3f, 4.f, 0.1f, UpdateVideoColorMenu, -1, -1, kMenuSliderValue);
CGameMenuItemSliderFloat itemOptionsDisplayColorContrast("CONTRAST:", 3, 66, 150, 180, &g_videoContrast, 0.1f, 2.7f, 0.05f, UpdateVideoColorMenu, -1, -1, kMenuSliderValue);
CGameMenuItemSliderFloat itemOptionsDisplayColorBrightness("BRIGHTNESS:", 3, 66, 160, 180, &g_videoBrightness, -0.8f, 0.8f, 0.05f, UpdateVideoColorMenu, -1, -1, kMenuSliderValue);
CGameMenuItemSliderFloat itemOptionsDisplayColorVisibility("VISIBILITY:", 3, 66, 170, 180, &r_ambientlight, 0.125f, 4.f, 0.125f, UpdateVideoColorMenu, -1, -1, kMenuSliderValue);
CGameMenuItemChain itemOptionsDisplayColorReset("RESET TO DEFAULTS", 3, 66, 180, 180, 0, NULL, 0, ResetVideoColor, 0);

const char *pzTextureModeStrings[] = {
    "CLASSIC",
    "FILTERED"
};

#ifdef USE_OPENGL
int nTextureModeValues[] = {
    TEXFILTER_OFF,
    TEXFILTER_ON
};
#endif

const char *pzAnisotropyStrings[] = {
    "MAX",
    "NONE",
    "2X",
    "4X",
    "8X",
    "16X"
};

int nAnisotropyValues[] = {
    0,
    1,
    2,
    4,
    8,
    16
};

const char *pzTexQualityStrings[] = {
    "FULL",
    "HALF",
    "BARF"
};


void UpdateTextureMode(CGameMenuItemZCycle *pItem);
void UpdateAnisotropy(CGameMenuItemZCycle *pItem);
void UpdateTrueColorTextures(CGameMenuItemZBool *pItem);
void UpdateTexQuality(CGameMenuItemZCycle *pItem);
void UpdatePreloadCache(CGameMenuItemZBool *pItem);
void UpdateDetailTex(CGameMenuItemZBool *pItem);
void UpdateGlowTex(CGameMenuItemZBool *pItem);
void Update3DModels(CGameMenuItemZBool *pItem);
void UpdateDeliriumBlur(CGameMenuItemZBool *pItem);
#ifdef USE_OPENGL
void PreDrawDisplayPolymost(CGameMenuItem *pItem);
CGameMenuItemTitle itemOptionsDisplayPolymostTitle("POLYMOST SETUP", 1, 160, 20, 2038);
CGameMenuItemZCycle itemOptionsDisplayPolymostTextureMode("TEXTURE MODE:", 3, 66, 60, 180, 0, UpdateTextureMode, pzTextureModeStrings, 2, 0);
CGameMenuItemZCycle itemOptionsDisplayPolymostAnisotropy("ANISOTROPY:", 3, 66, 70, 180, 0, UpdateAnisotropy, pzAnisotropyStrings, 6, 0);
CGameMenuItemZBool itemOptionsDisplayPolymostTrueColorTextures("TRUE COLOR TEXTURES:", 3, 66, 80, 180, 0, UpdateTrueColorTextures, NULL, NULL);
CGameMenuItemZCycle itemOptionsDisplayPolymostTexQuality("GL TEXTURE QUALITY:", 3, 66, 90, 180, 0, UpdateTexQuality, pzTexQualityStrings, 3, 0);
CGameMenuItemZBool itemOptionsDisplayPolymostPreloadCache("PRE-LOAD MAP TEXTURES:", 3, 66, 100, 180, 0, UpdatePreloadCache, NULL, NULL);
CGameMenuItemZBool itemOptionsDisplayPolymostDetailTex("DETAIL TEXTURES:", 3, 66, 120, 180, 0, UpdateDetailTex, NULL, NULL);
CGameMenuItemZBool itemOptionsDisplayPolymostGlowTex("GLOW TEXTURES:", 3, 66, 130, 180, 0, UpdateGlowTex, NULL, NULL);
CGameMenuItemZBool itemOptionsDisplayPolymost3DModels("3D MODELS:", 3, 66, 140, 180, 0, Update3DModels, NULL, NULL);
CGameMenuItemZBool itemOptionsDisplayPolymostDeliriumBlur("DELIRIUM EFFECT BLUR:", 3, 66, 150, 180, 0, UpdateDeliriumBlur, NULL, NULL);
#endif

void UpdateSoundToggle(CGameMenuItemZBool *pItem);
void UpdateMusicToggle(CGameMenuItemZBool *pItem);
void Update3DToggle(CGameMenuItemZBool *pItem);
void UpdateCDToggle(CGameMenuItemZBool *pItem);
void UpdateSoundVolume(CGameMenuItemSlider *pItem);
void UpdateMusicVolume(CGameMenuItemSlider *pItem);
void UpdateSoundRate(CGameMenuItemZCycle *pItem);
void UpdateNumVoices(CGameMenuItemSlider *pItem);
void UpdateMusicDevice(CGameMenuItemZCycle *pItem);
void SetSound(CGameMenuItemChain *pItem);
void PreDrawSound(CGameMenuItem *pItem);
const char *pzSoundRateStrings[] = {
    "22050HZ",
    "44100HZ",
    "48000HZ"
};

int nSoundRateValues[] = {
    22050,
    44100,
    48000
};

const char *pzMusicDeviceStrings[] = {
    "SYSTEM MIDI",
    "OPL3(SB/ADLIB)"
};

CGameMenuItemTitle itemOptionsSoundTitle("SOUND SETUP", 1, 160, 20, 2038);
CGameMenuItemZBool itemOptionsSoundSoundToggle("SOUND:", 3, 66, 60, 180, false, UpdateSoundToggle, NULL, NULL);
CGameMenuItemZBool itemOptionsSoundMusicToggle("MUSIC:", 3, 66, 70, 180, false, UpdateMusicToggle, NULL, NULL);
CGameMenuItemZBool itemOptionsSound3DToggle("3D AUDIO:", 3, 66, 80, 180, false, Update3DToggle, NULL, NULL);
CGameMenuItemSlider itemOptionsSoundSoundVolume("SOUND VOLUME:", 3, 66, 90, 180, &FXVolume, 0, 256, 48, UpdateSoundVolume, -1, -1, kMenuSliderPercent);
CGameMenuItemSlider itemOptionsSoundMusicVolume("MUSIC VOLUME:", 3, 66, 100, 180, &MusicVolume, 0, 256, 48, UpdateMusicVolume, -1, -1, kMenuSliderPercent);
CGameMenuItemZCycle itemOptionsSoundSampleRate("SAMPLE RATE:", 3, 66, 110, 180, 0, UpdateSoundRate, pzSoundRateStrings, 3, 0);
CGameMenuItemSlider itemOptionsSoundNumVoices("VOICES:", 3, 66, 120, 180, NumVoices, 16, 256, 16, UpdateNumVoices, -1, -1, kMenuSliderValue);
CGameMenuItemZBool itemOptionsSoundCDToggle("REDBOOK AUDIO:", 3, 66, 130, 180, false, UpdateCDToggle, NULL, NULL);
CGameMenuItemZCycle itemOptionsSoundMusicDevice("MUSIC DEVICE:", 3, 66, 140, 180, 0, UpdateMusicDevice, pzMusicDeviceStrings, 2, 0);
CGameMenuItemChain itemOptionsSoundApplyChanges("APPLY CHANGES", 3, 66, 150, 180, 0, NULL, 0, SetSound, 0);


void UpdatePlayerName(CGameMenuItemZEdit *pItem, CGameMenuEvent *pEvent);

CGameMenuItemTitle itemOptionsPlayerTitle("PLAYER SETUP", 1, 160, 20, 2038);
CGameMenuItemZEdit itemOptionsPlayerName("PLAYER NAME:", 3, 66, 60, 180, szPlayerName, MAXPLAYERNAME, 0, UpdatePlayerName, 0);

CGameMenu menuOptionsControlKeyboard;
CGameMenu menuOptionsControlMouse;
CGameMenu menuOptionsControlMouseButtonAssignment;

void SetupMouseMenu(CGameMenuItemChain *pItem);

CGameMenuItemTitle itemOptionsControlTitle("CONTROL SETUP", 1, 160, 20, 2038);
CGameMenuItemChain itemOptionsControlKeyboard("KEYBOARD SETUP", 1, 0, 60, 320, 1, &menuOptionsControlKeyboard, -1, NULL, 0);
CGameMenuItemChain itemOptionsControlMouse("MOUSE SETUP", 1, 0, 80, 320, 1, &menuOptionsControlMouse, -1, SetupMouseMenu, 0);

CGameMenuItemTitle itemOptionsControlKeyboardTitle("KEYBOARD SETUP", 1, 160, 20, 2038);
CGameMenuItemChain itemOptionsControlKeyboardList("Configure Keys...", 1, 0, 60, 320, 1, &menuKeys, -1, NULL, 0);
CGameMenuItemChain itemOptionsControlKeyboardReset("Reset Keys (default)...", 1, 0, 80, 320, 1, &menuKeys, -1, ResetKeys, 0);
CGameMenuItemChain itemOptionsControlKeyboardResetClassic("Reset Keys (classic)...", 1, 0, 100, 320, 1, &menuKeys, -1, ResetKeysClassic, 0);

void SetMouseFilterInput(CGameMenuItemZBool *pItem);
void SetMouseAimMode(CGameMenuItemZBool *pItem);
void SetMouseVerticalAim(CGameMenuItemZBool *pItem);
void SetMouseXScale(CGameMenuItemSlider *pItem);
void SetMouseYScale(CGameMenuItemSlider *pItem);
void SetMouseDigitalAxis(CGameMenuItemZCycle *pItem);

void PreDrawControlMouse(CGameMenuItem *pItem);

void SetupMouseButtonMenu(CGameMenuItemChain *pItem);

CGameMenuItemTitle itemOptionsControlMouseTitle("MOUSE SETUP", 1, 160, 20, 2038);
CGameMenuItemChain itemOptionsControlMouseButton("BUTTON ASSIGNMENT", 3, 66, 60, 180, 0, &menuOptionsControlMouseButtonAssignment, 0, SetupMouseButtonMenu, 0);
CGameMenuItemSliderFloat itemOptionsControlMouseSensitivity("SENSITIVITY:", 3, 66, 70, 180, &CONTROL_MouseSensitivity, 0.5f, 16.f, 0.5f, SetMouseSensitivity, -1, -1, kMenuSliderValue);
CGameMenuItemZBool itemOptionsControlMouseAimFlipped("INVERT AIMING:", 3, 66, 80, 180, false, SetMouseAimFlipped, NULL, NULL);
CGameMenuItemZBool itemOptionsControlMouseFilterInput("FILTER INPUT:", 3, 66, 90, 180, false, SetMouseFilterInput, NULL, NULL);
CGameMenuItemZBool itemOptionsControlMouseAimMode("AIMING TYPE:", 3, 66, 100, 180, false, SetMouseAimMode, "HOLD", "TOGGLE");
CGameMenuItemZBool itemOptionsControlMouseVerticalAim("VERTICAL AIMING:", 3, 66, 110, 180, false, SetMouseVerticalAim, NULL, NULL);
CGameMenuItemSlider itemOptionsControlMouseXScale("X-SCALE:", 3, 66, 120, 180, (int*)&MouseAnalogueScale[0], 0, 65536, 1024, SetMouseXScale, -1, -1, kMenuSliderQ16);
CGameMenuItemSlider itemOptionsControlMouseYScale("Y-SCALE:", 3, 66, 130, 180, (int*)&MouseAnalogueScale[1], 0, 65536, 1024, SetMouseYScale, -1, -1, kMenuSliderQ16);
CGameMenuItemZCycle itemOptionsControlMouseDigitalUp("DIGITAL UP", 3, 66, 140, 180, 0, SetMouseDigitalAxis, NULL, 0, 0, true);
CGameMenuItemZCycle itemOptionsControlMouseDigitalDown("DIGITAL DOWN", 3, 66, 150, 180, 0, SetMouseDigitalAxis, NULL, 0, 0, true);
CGameMenuItemZCycle itemOptionsControlMouseDigitalLeft("DIGITAL LEFT", 3, 66, 160, 180, 0, SetMouseDigitalAxis, NULL, 0, 0, true);
CGameMenuItemZCycle itemOptionsControlMouseDigitalRight("DIGITAL RIGHT", 3, 66, 170, 180, 0, SetMouseDigitalAxis, NULL, 0, 0, true);

void SetupNetworkMenu(void);
void SetupNetworkHostMenu(CGameMenuItemChain *pItem);
void SetupNetworkJoinMenu(CGameMenuItemChain *pItem);
void NetworkHostGame(CGameMenuItemChain *pItem);
void NetworkJoinGame(CGameMenuItemChain *pItem);

char zNetAddressBuffer[16] = "localhost";
char zNetPortBuffer[6];

CGameMenuItemTitle itemNetworkTitle("MULTIPLAYER", 1, 160, 20, 2038);
CGameMenuItemChain itemNetworkHost("HOST A GAME", 1, 0, 80, 320, 1, &menuNetworkHost, -1, SetupNetworkHostMenu, 0);
CGameMenuItemChain itemNetworkJoin("JOIN A GAME", 1, 0, 100, 320, 1, &menuNetworkJoin, -1, SetupNetworkJoinMenu, 0);

CGameMenuItemTitle itemNetworkHostTitle("HOST A GAME", 1, 160, 20, 2038);
CGameMenuItemSlider itemNetworkHostPlayerNum("PLAYER NUMBER:", 3, 66, 70, 180, 1, 2, kMaxPlayers, 1, NULL, -1, -1, kMenuSliderValue);
CGameMenuItemZEdit itemNetworkHostPort("NETWORK PORT:", 3, 66, 80, 180, zNetPortBuffer, 6, 0, NULL, 0);
CGameMenuItemChain itemNetworkHostHost("HOST A GAME", 3, 66, 100, 180, 1, NULL, -1, NetworkHostGame, 0);

CGameMenuItemTitle itemNetworkJoinTitle("JOIN A GAME", 1, 160, 20, 2038);
CGameMenuItemZEdit itemNetworkJoinAddress("NETWORK ADDRESS:", 3, 66, 70, 180, zNetAddressBuffer, 16, 0, NULL, 0);
CGameMenuItemZEdit itemNetworkJoinPort("NETWORK PORT:", 3, 66, 80, 180, zNetPortBuffer, 6, 0, NULL, 0);
CGameMenuItemChain itemNetworkJoinJoin("JOIN A GAME", 3, 66, 100, 180, 1, NULL, -1, NetworkJoinGame, 0);

// There is no better way to do this than manually.

#define MENUMOUSEFUNCTIONS 12

static char const *MenuMouseNames[MENUMOUSEFUNCTIONS] = {
    "Button 1",
    "Double Button 1",
    "Button 2",
    "Double Button 2",
    "Button 3",
    "Double Button 3",

    "Wheel Up",
    "Wheel Down",

    "Button 4",
    "Double Button 4",
    "Button 5",
    "Double Button 5",
};

static int32_t MenuMouseDataIndex[MENUMOUSEFUNCTIONS][2] = {
    { 0, 0, },
    { 0, 1, },
    { 1, 0, },
    { 1, 1, },
    { 2, 0, },
    { 2, 1, },

    // note the mouse wheel
    { 4, 0, },
    { 5, 0, },

    { 3, 0, },
    { 3, 1, },
    { 6, 0, },
    { 6, 1, },
};

void SetMouseButton(CGameMenuItemZCycle *pItem);

CGameMenuItemZCycle *pItemOptionsControlMouseButton[MENUMOUSEFUNCTIONS];

void SetupLoadingScreen(void)
{
    menuLoading.Add(&itemLoadingText, true);
}

void SetupKeyListMenu(void)
{
    menuKeys.Add(&itemKeysTitle, false);
    menuKeys.Add(&itemKeyList, true);
    menuKeys.Add(&itemBloodQAV, false);
}

void SetupMessagesMenu(void)
{
    menuMessages.Add(&itemMessagesTitle, false);
    menuMessages.Add(&boolMessages, true);
    menuMessages.Add(&sliderMsgCount, false);
    menuMessages.Add(&sliderMsgTime, false);
    menuMessages.Add(&boolMsgFont, false);
    menuMessages.Add(&boolMsgIncoming, false);
    menuMessages.Add(&boolMsgSelf, false);
    menuMessages.Add(&boolMsgOther, false);
    menuMessages.Add(&boolMsgRespawn, false);
    menuMessages.Add(&itemBloodQAV, false);
}

void SetupControlsMenu(void)
{
    sliderMouseSpeed.fValue = ClipRangeF(CONTROL_MouseSensitivity, sliderMouseSpeed.fRangeLow, sliderMouseSpeed.fRangeHigh);
    sliderTurnSpeed.nValue = ClipRange(gTurnSpeed, sliderTurnSpeed.nRangeLow, sliderTurnSpeed.nRangeHigh);
    boolMouseFlipped.at20 = gMouseAimingFlipped;
    menuControls.Add(&itemControlsTitle, false);
    menuControls.Add(&sliderMouseSpeed, true);
    menuControls.Add(&boolMouseFlipped, false);
    menuControls.Add(&sliderTurnSpeed, false);
    menuControls.Add(&itemChainKeyList, false);
    menuControls.Add(&itemChainKeyReset, false);
    menuControls.Add(&itemChainKeyResetClassic, false);
    menuControls.Add(&itemBloodQAV, false);
}

void SetupOptionsOldMenu(void)
{
    sliderDetail.nValue = ClipRange(gDetail, sliderDetail.nRangeLow, sliderDetail.nRangeHigh);
    sliderGamma.nValue = ClipRange(gGamma, sliderGamma.nRangeLow, sliderGamma.nRangeHigh);
    sliderMusic.nValue = ClipRange(MusicVolume, sliderMusic.nRangeLow, sliderMusic.nRangeHigh);
    sliderSound.nValue = ClipRange(FXVolume, sliderSound.nRangeLow, sliderSound.nRangeHigh);
    bool3DAudio.at20 = gDoppler;
    boolCrosshair.at20 = cl_crosshair;
    boolShowWeapons.at20 = gShowWeapon;
    boolSlopeTilting.at20 = gSlopeTilting;
    boolViewBobbing.at20 = gViewVBobbing;
    boolViewSwaying.at20 = gViewHBobbing;
    boolMessages.at20 = gGameMessageMgr.state;
    menuOptionsOld.Add(&itemOptionsTitle, false);
    menuOptionsOld.Add(&itemOption1, true);
    menuOptionsOld.Add(&sliderDetail, false);
    menuOptionsOld.Add(&sliderGamma, false);
    menuOptionsOld.Add(&sliderMusic, false);
    menuOptionsOld.Add(&sliderSound, false);
    menuOptionsOld.Add(&sliderCDAudio, false);
    menuOptionsOld.Add(&bool3DAudio, false);
    menuOptionsOld.Add(&boolCrosshair, false);
    menuOptionsOld.Add(&boolShowWeapons, false);
    menuOptionsOld.Add(&boolSlopeTilting, false);
    menuOptionsOld.Add(&boolViewBobbing, false);
    menuOptionsOld.Add(&boolViewSwaying, false);
    menuOptionsOld.Add(&itemOption2, false);
    menuOptionsOld.Add(&itemChainParentalLock, false);
    menuOptionsOld.Add(&itemBloodQAV, false);
}

void SetupDifficultyMenu(void)
{
    menuDifficulty.Add(&itemDifficultyTitle, false);
    menuDifficulty.Add(&itemDifficulty1, false);
    menuDifficulty.Add(&itemDifficulty2, false);
    menuDifficulty.Add(&itemDifficulty3, true);
    menuDifficulty.Add(&itemDifficulty4, false);
    menuDifficulty.Add(&itemDifficulty5, false);
    menuDifficulty.Add(&itemBloodQAV, false);
}

void SetupEpisodeMenu(void)
{
    menuEpisode.Add(&itemEpisodesTitle, false);
    bool unk = false;
    int height;
    gMenuTextMgr.GetFontInfo(1, NULL, NULL, &height);
    int j = 0;
    for (int i = 0; i < 6; i++)
    {
        EPISODEINFO *pEpisode = &gEpisodeInfo[i];
        if (!pEpisode->bloodbath || gGameOptions.nGameType != 0)
        {
            if (j < gEpisodeCount)
            {
                CGameMenuItemChain7F2F0 *pEpisodeItem = &itemEpisodes[j];
                pEpisodeItem->m_nFont = 1;
                pEpisodeItem->m_nX = 0;
                pEpisodeItem->m_nWidth = 320;
                pEpisodeItem->at20 = 1;
                pEpisodeItem->m_pzText = pEpisode->at0;
                pEpisodeItem->m_nY = 55+(height+8)*j;
                pEpisodeItem->at34 = i;
                if (!unk || j == 0)
                {
                    pEpisodeItem = &itemEpisodes[j];
                    pEpisodeItem->at24 = &menuDifficulty;
                    pEpisodeItem->at28 = 3;
                }
                else
                {
                    pEpisodeItem->at24 = &menuSorry2;
                    pEpisodeItem->at28 = 1;
                }
                pEpisodeItem = &itemEpisodes[j];
                pEpisodeItem->bCanSelect = 1;
                pEpisodeItem->bEnable = 1;
                bool first = j == 0;
                menuEpisode.Add(&itemEpisodes[j], first);
                if (first)
                    SetupLevelMenuItem(j);
            }
            j++;
        }
    }
    menuEpisode.Add(&itemBloodQAV, false);
}

void SetupMainMenu(void)
{
    menuMain.Add(&itemMainTitle, false);
    menuMain.Add(&itemMain1, true);
    if (gGameOptions.nGameType > 0)
    {
        itemMain1.at24 = &menuNetStart;
        itemMain1.at28 = 2;
    }
    else
    {
        itemMain1.at24 = &menuEpisode;
        itemMain1.at28 = -1;
    }
    menuMain.Add(&itemMain2, false);
    menuMain.Add(&itemMain3, false);
    menuMain.Add(&itemMain4, false);
    menuMain.Add(&itemMain5, false);
    menuMain.Add(&itemMain6, false);
    menuMain.Add(&itemMain7, false);
    menuMain.Add(&itemBloodQAV, false);
}

void SetupMainMenuWithSave(void)
{
    menuMainWithSave.Add(&itemMainSaveTitle, false);
    menuMainWithSave.Add(&itemMainSave1, true);
    if (gGameOptions.nGameType > 0)
    {
        itemMainSave1.at24 = &menuNetStart;
        itemMainSave1.at28 = 2;
    }
    else
    {
        itemMainSave1.at24 = &menuEpisode;
        itemMainSave1.at28 = -1;
    }
    menuMainWithSave.Add(&itemMainSave2, false);
    menuMainWithSave.Add(&itemMainSave3, false);
    menuMainWithSave.Add(&itemMainSave4, false);
    menuMainWithSave.Add(&itemMainSave5, false);
    menuMainWithSave.Add(&itemMainSave6, false);
    menuMainWithSave.Add(&itemMainSave7, false);
    menuMainWithSave.Add(&itemMainSave8, false);
    menuMainWithSave.Add(&itemBloodQAV, false);
}

void SetupNetStartMenu(void)
{
    bool oneEpisode = false;
    menuNetStart.Add(&itemNetStartTitle, false);
    menuNetStart.Add(&itemNetStart1, false);
    for (int i = 0; i < (oneEpisode ? 1 : 6); i++)
    {
        EPISODEINFO *pEpisode = &gEpisodeInfo[i];
        if (i < gEpisodeCount)
            itemNetStart2.Add(pEpisode->at0, i == 0);
    }
    menuNetStart.Add(&itemNetStart2, false);
    menuNetStart.Add(&itemNetStart3, false);
    menuNetStart.Add(&itemNetStart4, false);
    menuNetStart.Add(&itemNetStart5, false);
    menuNetStart.Add(&itemNetStart6, false);
    menuNetStart.Add(&itemNetStart7, false);
    menuNetStart.Add(&itemNetStart8, false);
    menuNetStart.Add(&itemNetStart9, false);
    menuNetStart.Add(&itemNetStart10, false);
    menuNetStart.Add(&itemNetStart11, false);
    menuNetStart.Add(&itemNetStart12, false);
    itemNetStart1.SetTextIndex(1);
    itemNetStart4.SetTextIndex(2);
    itemNetStart5.SetTextIndex(0);
    itemNetStart6.SetTextIndex(1);
    itemNetStart7.SetTextIndex(1);
    menuNetStart.Add(&itemBloodQAV, false);
}

void SetupSaveGameMenu(void)
{
    menuSaveGame.Add(&itemSaveTitle, false);
    menuSaveGame.Add(&itemSaveGame1, true);
    menuSaveGame.Add(&itemSaveGame2, false);
    menuSaveGame.Add(&itemSaveGame3, false);
    menuSaveGame.Add(&itemSaveGame4, false);
    menuSaveGame.Add(&itemSaveGame5, false);
    menuSaveGame.Add(&itemSaveGame6, false);
    menuSaveGame.Add(&itemSaveGame7, false);
    menuSaveGame.Add(&itemSaveGame8, false);
    menuSaveGame.Add(&itemSaveGame9, false);
    menuSaveGame.Add(&itemSaveGame10, false);
    menuSaveGame.Add(&itemSaveGamePic, false);
    menuSaveGame.Add(&itemBloodQAV, false);

    itemSaveGame1.at2c = &itemSaveGamePic;
    if (!strcmp(strRestoreGameStrings[0], "<Empty>"))
        itemSaveGame1.at37 = 1;

    itemSaveGame2.at2c = &itemSaveGamePic;
    if (!strcmp(strRestoreGameStrings[1], "<Empty>"))
        itemSaveGame2.at37 = 1;

    itemSaveGame3.at2c = &itemSaveGamePic;
    if (!strcmp(strRestoreGameStrings[2], "<Empty>"))
        itemSaveGame3.at37 = 1;

    itemSaveGame4.at2c = &itemSaveGamePic;
    if (!strcmp(strRestoreGameStrings[3], "<Empty>"))
        itemSaveGame4.at37 = 1;

    itemSaveGame5.at2c = &itemSaveGamePic;
    if (!strcmp(strRestoreGameStrings[4], "<Empty>"))
        itemSaveGame5.at37 = 1;

    itemSaveGame6.at2c = &itemSaveGamePic;
    if (!strcmp(strRestoreGameStrings[5], "<Empty>"))
        itemSaveGame6.at37 = 1;

    itemSaveGame7.at2c = &itemSaveGamePic;
    if (!strcmp(strRestoreGameStrings[6], "<Empty>"))
        itemSaveGame7.at37 = 1;

    itemSaveGame8.at2c = &itemSaveGamePic;
    if (!strcmp(strRestoreGameStrings[7], "<Empty>"))
        itemSaveGame8.at37 = 1;

    itemSaveGame9.at2c = &itemSaveGamePic;
    if (!strcmp(strRestoreGameStrings[8], "<Empty>"))
        itemSaveGame9.at37 = 1;

    itemSaveGame10.at2c = &itemSaveGamePic;
    if (!strcmp(strRestoreGameStrings[9], "<Empty>"))
        itemSaveGame10.at37 = 1;
}

void SetupLoadGameMenu(void)
{
    menuLoadGame.Add(&itemLoadTitle, false);
    menuLoadGame.Add(&itemLoadGame1, true);
    menuLoadGame.Add(&itemLoadGame2, false);
    menuLoadGame.Add(&itemLoadGame3, false);
    menuLoadGame.Add(&itemLoadGame4, false);
    menuLoadGame.Add(&itemLoadGame5, false);
    menuLoadGame.Add(&itemLoadGame6, false);
    menuLoadGame.Add(&itemLoadGame7, false);
    menuLoadGame.Add(&itemLoadGame8, false);
    menuLoadGame.Add(&itemLoadGame9, false);
    menuLoadGame.Add(&itemLoadGame10, false);
    menuLoadGame.Add(&itemLoadGamePic, false);
    itemLoadGame1.at35 = 0;
    itemLoadGame2.at35 = 0;
    itemLoadGame3.at35 = 0;
    itemLoadGame4.at35 = 0;
    itemLoadGame5.at35 = 0;
    itemLoadGame6.at35 = 0;
    itemLoadGame7.at35 = 0;
    itemLoadGame8.at35 = 0;
    itemLoadGame9.at35 = 0;
    itemLoadGame10.at35 = 0;
    itemLoadGame1.at2c = &itemLoadGamePic;
    itemLoadGame2.at2c = &itemLoadGamePic;
    itemLoadGame3.at2c = &itemLoadGamePic;
    itemLoadGame4.at2c = &itemLoadGamePic;
    itemLoadGame5.at2c = &itemLoadGamePic;
    itemLoadGame6.at2c = &itemLoadGamePic;
    itemLoadGame7.at2c = &itemLoadGamePic;
    itemLoadGame8.at2c = &itemLoadGamePic;
    itemLoadGame9.at2c = &itemLoadGamePic;
    itemLoadGame10.at2c = &itemLoadGamePic;
    menuLoadGame.Add(&itemBloodQAV, false);
}

void SetupSoundsMenu(void)
{
    itemSoundsMusic.nValue = ClipRange(MusicVolume, itemSoundsMusic.nRangeLow, itemSoundsMusic.nRangeHigh);
    itemSoundsSound.nValue = ClipRange(FXVolume, itemSoundsSound.nRangeLow, itemSoundsSound.nRangeHigh);
    menuSounds.Add(&itemSoundsTitle, false);
    menuSounds.Add(&itemSoundsMusic, true);
    menuSounds.Add(&itemSoundsSound, false);
    menuSounds.Add(&itemSoundsCDAudio, false);
    menuSounds.Add(&itemSounds3DAudio, false);
    menuSounds.Add(&itemBloodQAV, false);
}

void SetupQuitMenu(void)
{
    menuQuit.Add(&itemQuitTitle, false);
    menuQuit.Add(&itemQuitText1, false);
    menuQuit.Add(&itemQuitYesNo, true);
    menuQuit.Add(&itemBloodQAV, false);

    menuRestart.Add(&itemRestartTitle, false);
    menuRestart.Add(&itemRestartText1, false);
    menuRestart.Add(&itemRestartYesNo, true);
    menuRestart.Add(&itemBloodQAV, false);
}

void SetupHelpOrderMenu(void)
{
    menuOrder.Add(&itemHelp4QAV, true);
    menuOrder.Add(&itemHelp5QAV, false);
    menuOrder.Add(&itemHelp3QAV, false);
    menuOrder.Add(&itemHelp3BQAV, false);
    itemHelp4QAV.bEnable = 1;
    itemHelp4QAV.bNoDraw = 1;
    itemHelp5QAV.bEnable = 1;
    itemHelp5QAV.bNoDraw = 1;
    itemHelp3QAV.bEnable = 1;
    itemHelp3QAV.bNoDraw = 1;
    itemHelp3BQAV.bEnable = 1;
    itemHelp3BQAV.bNoDraw = 1;
}

void SetupCreditsMenu(void)
{
    menuCredits.Add(&itemCreditsQAV, true);
    itemCreditsQAV.bEnable = 1;
    itemCreditsQAV.bNoDraw = 1;
}

void SetupParentalLockMenu(void)
{
    itemParentalLockToggle.at20 = gbAdultContent;
    strcpy(itemParentalLockPassword.at20, gzAdultPassword);
    menuParentalLock.Add(&itemParentalLockTitle, false);
    menuParentalLock.Add(&itemParentalLockToggle, true);
    menuParentalLock.Add(&itemParentalLockPassword, false);
    menuParentalLock.Add(&itemBloodQAV, false);
}

void SetupSorry3Menu(void)
{
    menuPlayOnline.Add(&unk_26E06C, false);
    menuPlayOnline.Add(&unk_26E090, true);
    menuPlayOnline.Add(&unk_26E0E8, false);
    menuPlayOnline.Add(&unk_26E140, false);
    menuPlayOnline.Add(&unk_26E198, false);
    menuPlayOnline.Add(&unk_26E1F0, false);
    menuPlayOnline.Add(&unk_26E248, false);
    menuPlayOnline.Add(&itemBloodQAV, false);
}

void SetupSorryMenu(void)
{
    menuSorry.Add(&itemSorryPicCycle, true);
    menuSorry.Add(&itemSorryText1, false);
    menuSorry.Add(&itemSorryText3, false);
    menuSorry.Add(&itemBloodQAV, false);
}

void SetupSorry2Menu(void)
{
    menuSorry2.Add(&itemSorryPicCycle, true);
    menuSorry2.Add(&itemSorry2Text1, false);
    menuSorry2.Add(&itemSorry2Text2, false);
    menuSorry2.Add(&itemSorry2Text3, false);
    menuSorry2.Add(&itemBloodQAV, false);
}

void SetupOptionsMenu(void)
{
    menuOptions.Add(&itemOptionsTitle, false);
    menuOptions.Add(&itemOptionsChainGame, true);
    menuOptions.Add(&itemOptionsChainDisplay, false);
    menuOptions.Add(&itemOptionsChainSound, false);
    menuOptions.Add(&itemOptionsChainPlayer, false);
    menuOptions.Add(&itemOptionsChainControl, false);
    //menuOptions.Add(&itemOptionsChainOld, false);
    menuOptions.Add(&itemBloodQAV, false);

    menuOptionsGame.Add(&itemOptionsGameTitle, false);
    menuOptionsGame.Add(&itemOptionsGameBoolShowWeapons, true);
    menuOptionsGame.Add(&itemOptionsGameBoolSlopeTilting, false);
    menuOptionsGame.Add(&itemOptionsGameBoolViewBobbing, false);
    menuOptionsGame.Add(&itemOptionsGameBoolViewSwaying, false);
    menuOptionsGame.Add(&itemOptionsGameBoolAutoAim, false);
    menuOptionsGame.Add(&itemOptionsGameWeaponSwitch, false);

    //////////////////////
    if (gGameOptions.nGameType == 0) {
        menuOptionsGame.Add(&itemOptionsGameBoolWeaponsV10X, false);
    }
    /////////////////////

    //menuOptionsGame.Add(&itemOptionsGameChainParentalLock, false);
    menuOptionsGame.Add(&itemBloodQAV, false);
    itemOptionsGameBoolShowWeapons.at20 = gShowWeapon;
    itemOptionsGameBoolSlopeTilting.at20 = gSlopeTilting;
    itemOptionsGameBoolViewBobbing.at20 = gViewVBobbing;
    itemOptionsGameBoolViewSwaying.at20 = gViewHBobbing;
    itemOptionsGameBoolAutoAim.m_nFocus = gAutoAim;
    itemOptionsGameWeaponSwitch.m_nFocus = (gWeaponSwitch&1) ? ((gWeaponSwitch&2) ? 1 : 2) : 0;

    ///////
    itemOptionsGameBoolWeaponsV10X.at20 = gWeaponsV10x;
    ///////

    menuOptionsDisplay.Add(&itemOptionsDisplayTitle, false);
    menuOptionsDisplay.Add(&itemOptionsDisplayColor, true);
    menuOptionsDisplay.Add(&itemOptionsDisplayMode, false);
    menuOptionsDisplay.Add(&itemOptionsDisplayBoolCrosshair, false);
    menuOptionsDisplay.Add(&itemOptionsDisplayBoolCenterHoriz, false);
    menuOptionsDisplay.Add(&itemOptionsDisplayBoolLevelStats, false);
    menuOptionsDisplay.Add(&itemOptionsDisplayBoolPowerupDuration, false);
    menuOptionsDisplay.Add(&itemOptionsDisplayBoolShowMapTitle, false);
    menuOptionsDisplay.Add(&itemOptionsDisplayBoolMessages, false);
    menuOptionsDisplay.Add(&itemOptionsDisplayBoolWidescreen, false);
    menuOptionsDisplay.Add(&itemOptionsDisplayFOV, false);
#ifdef USE_OPENGL
    menuOptionsDisplay.Add(&itemOptionsDisplayPolymost, false);
#endif
    menuOptionsDisplay.Add(&itemBloodQAV, false);
    itemOptionsDisplayBoolCrosshair.at20 = cl_crosshair;
    itemOptionsDisplayBoolCenterHoriz.at20 = gCenterHoriz;
    itemOptionsDisplayBoolLevelStats.at20 = gLevelStats;
    itemOptionsDisplayBoolPowerupDuration.at20 = gPowerupDuration;
    itemOptionsDisplayBoolShowMapTitle.at20 = gShowMapTitle;
    itemOptionsDisplayBoolMessages.at20 = gMessageState;
    itemOptionsDisplayBoolWidescreen.at20 = r_usenewaspect;

    menuOptionsDisplayMode.Add(&itemOptionsDisplayModeTitle, false);
    menuOptionsDisplayMode.Add(&itemOptionsDisplayModeResolution, true);
    // prepare video setup
    for (int i = 0; i < validmodecnt; ++i)
    {
        int j;
        for (j = 0; j < gResolutionNum; ++j)
        {
            if (validmode[i].xdim == gResolution[j].xdim && validmode[i].ydim == gResolution[j].ydim)
            {
                gResolution[j].flags |= validmode[i].fs ? RES_FS : RES_WIN;
                Bsnprintf(gResolution[j].name, MAXRESOLUTIONSTRINGLENGTH, "%d x %d%s", gResolution[j].xdim, gResolution[j].ydim, (gResolution[j].flags & RES_FS) ? "" : "Win");
                gResolutionName[j] = gResolution[j].name;
                if (validmode[i].bpp > gResolution[j].bppmax)
                    gResolution[j].bppmax = validmode[i].bpp;
                break;
            }
        }

        if (j == gResolutionNum) // no match found
        {
            gResolution[j].xdim = validmode[i].xdim;
            gResolution[j].ydim = validmode[i].ydim;
            gResolution[j].bppmax = validmode[i].bpp;
            gResolution[j].flags = validmode[i].fs ? RES_FS : RES_WIN;
            Bsnprintf(gResolution[j].name, MAXRESOLUTIONSTRINGLENGTH, "%d x %d%s", gResolution[j].xdim, gResolution[j].ydim, (gResolution[j].flags & RES_FS) ? "" : "Win");
            gResolutionName[j] = gResolution[j].name;
            ++gResolutionNum;
        }
    }
    itemOptionsDisplayModeResolution.SetTextArray(gResolutionName, gResolutionNum, 0);
#ifdef USE_OPENGL
    menuOptionsDisplayMode.Add(&itemOptionsDisplayModeRenderer, false);
#endif
    menuOptionsDisplayMode.Add(&itemOptionsDisplayModeFullscreen, false);
    menuOptionsDisplayMode.Add(&itemOptionsDisplayModeVSync, false);
    menuOptionsDisplayMode.Add(&itemOptionsDisplayModeFrameLimit, false);
    menuOptionsDisplayMode.Add(&itemOptionsDisplayModeFPSOffset, false);
    menuOptionsDisplayMode.Add(&itemOptionsDisplayModeApply, false);
    menuOptionsDisplayMode.Add(&itemBloodQAV, false);

#ifdef USE_OPENGL
    itemOptionsDisplayModeRenderer.pPreDrawCallback = PreDrawVideoModeMenu;
#endif
    itemOptionsDisplayModeFullscreen.pPreDrawCallback = PreDrawVideoModeMenu;
    itemOptionsDisplayModeFPSOffset.pPreDrawCallback = PreDrawVideoModeMenu;

    menuOptionsDisplayColor.Add(&itemOptionsDisplayColorTitle, false);
    menuOptionsDisplayColor.Add(&itemOptionsDisplayColorGamma, true);
    menuOptionsDisplayColor.Add(&itemOptionsDisplayColorContrast, false);
    menuOptionsDisplayColor.Add(&itemOptionsDisplayColorBrightness, false);
    menuOptionsDisplayColor.Add(&itemOptionsDisplayColorVisibility, false);
    menuOptionsDisplayColor.Add(&itemOptionsDisplayColorReset, false);
    menuOptionsDisplayColor.Add(&itemBloodQAV, false);

    itemOptionsDisplayColorContrast.pPreDrawCallback = PreDrawDisplayColor;
    itemOptionsDisplayColorBrightness.pPreDrawCallback = PreDrawDisplayColor;

#ifdef USE_OPENGL
    menuOptionsDisplayPolymost.Add(&itemOptionsDisplayPolymostTitle, false);
    //menuOptionsDisplayPolymost.Add(&itemOptionsDisplayPolymostTextureMode, true);
    //menuOptionsDisplayPolymost.Add(&itemOptionsDisplayPolymostAnisotropy, false);
    menuOptionsDisplayPolymost.Add(&itemOptionsDisplayPolymostTrueColorTextures, true);
    menuOptionsDisplayPolymost.Add(&itemOptionsDisplayPolymostTexQuality, false);
    menuOptionsDisplayPolymost.Add(&itemOptionsDisplayPolymostPreloadCache, false);
    menuOptionsDisplayPolymost.Add(&itemOptionsDisplayPolymostDetailTex, false);
    menuOptionsDisplayPolymost.Add(&itemOptionsDisplayPolymostGlowTex, false);
    menuOptionsDisplayPolymost.Add(&itemOptionsDisplayPolymost3DModels, false);
    menuOptionsDisplayPolymost.Add(&itemOptionsDisplayPolymostDeliriumBlur, false);
    menuOptionsDisplayPolymost.Add(&itemBloodQAV, false);

    itemOptionsDisplayPolymostTexQuality.pPreDrawCallback = PreDrawDisplayPolymost;
    itemOptionsDisplayPolymostPreloadCache.pPreDrawCallback = PreDrawDisplayPolymost;
    itemOptionsDisplayPolymostDetailTex.pPreDrawCallback = PreDrawDisplayPolymost;
    itemOptionsDisplayPolymostGlowTex.pPreDrawCallback = PreDrawDisplayPolymost;
#endif

    menuOptionsSound.Add(&itemOptionsSoundTitle, false);
    menuOptionsSound.Add(&itemOptionsSoundSoundToggle, true);
    menuOptionsSound.Add(&itemOptionsSoundMusicToggle, false);
    menuOptionsSound.Add(&itemOptionsSound3DToggle, false);
    menuOptionsSound.Add(&itemOptionsSoundSoundVolume, false);
    menuOptionsSound.Add(&itemOptionsSoundMusicVolume, false);
    menuOptionsSound.Add(&itemOptionsSoundSampleRate, false);
    menuOptionsSound.Add(&itemOptionsSoundNumVoices, false);
    menuOptionsSound.Add(&itemOptionsSoundCDToggle, false);
    menuOptionsSound.Add(&itemOptionsSoundMusicDevice, false);
    menuOptionsSound.Add(&itemOptionsSoundApplyChanges, false);
    menuOptionsSound.Add(&itemBloodQAV, false);

    menuOptionsPlayer.Add(&itemOptionsPlayerTitle, false);
    menuOptionsPlayer.Add(&itemOptionsPlayerName, true);
    menuOptionsPlayer.Add(&itemBloodQAV, false);

    menuOptionsControl.Add(&itemOptionsControlTitle, false);
    menuOptionsControl.Add(&itemOptionsControlKeyboard, true);
    menuOptionsControl.Add(&itemOptionsControlMouse, false);
    menuOptionsControl.Add(&itemBloodQAV, false);

    menuOptionsControlKeyboard.Add(&itemOptionsControlKeyboardTitle, false);
    menuOptionsControlKeyboard.Add(&itemOptionsControlKeyboardList, true);
    menuOptionsControlKeyboard.Add(&itemOptionsControlKeyboardReset, false);
    menuOptionsControlKeyboard.Add(&itemOptionsControlKeyboardResetClassic, false);
    menuOptionsControlKeyboard.Add(&itemBloodQAV, false);

    menuOptionsControlMouse.Add(&itemOptionsControlMouseTitle, false);
    menuOptionsControlMouse.Add(&itemOptionsControlMouseButton, true);
    menuOptionsControlMouse.Add(&itemOptionsControlMouseSensitivity, false);
    menuOptionsControlMouse.Add(&itemOptionsControlMouseAimFlipped, false);
    menuOptionsControlMouse.Add(&itemOptionsControlMouseFilterInput, false);
    menuOptionsControlMouse.Add(&itemOptionsControlMouseAimMode, false);
    menuOptionsControlMouse.Add(&itemOptionsControlMouseVerticalAim, false);
    menuOptionsControlMouse.Add(&itemOptionsControlMouseXScale, false);
    menuOptionsControlMouse.Add(&itemOptionsControlMouseYScale, false);
    menuOptionsControlMouse.Add(&itemOptionsControlMouseDigitalUp, false);
    menuOptionsControlMouse.Add(&itemOptionsControlMouseDigitalDown, false);
    menuOptionsControlMouse.Add(&itemOptionsControlMouseDigitalLeft, false);
    menuOptionsControlMouse.Add(&itemOptionsControlMouseDigitalRight, false);
    menuOptionsControlMouse.Add(&itemBloodQAV, false);

    itemOptionsControlMouseDigitalUp.SetTextArray(pzGamefuncsStrings, NUMGAMEFUNCTIONS+1, 0);
    itemOptionsControlMouseDigitalDown.SetTextArray(pzGamefuncsStrings, NUMGAMEFUNCTIONS+1, 0);
    itemOptionsControlMouseDigitalLeft.SetTextArray(pzGamefuncsStrings, NUMGAMEFUNCTIONS+1, 0);
    itemOptionsControlMouseDigitalRight.SetTextArray(pzGamefuncsStrings, NUMGAMEFUNCTIONS+1, 0);

    itemOptionsControlMouseVerticalAim.pPreDrawCallback = PreDrawControlMouse;

    menuOptionsControlMouseButtonAssignment.Add(&itemOptionsControlMouseTitle, false);
    int y = 60;
    for (int i = 0; i < MENUMOUSEFUNCTIONS; i++)
    {
        pItemOptionsControlMouseButton[i] = new CGameMenuItemZCycle(MenuMouseNames[i], 3, 66, y, 180, 0, SetMouseButton, pzGamefuncsStrings, NUMGAMEFUNCTIONS+1, 0, true);
        dassert(pItemOptionsControlMouseButton[i] != NULL);
        menuOptionsControlMouseButtonAssignment.Add(pItemOptionsControlMouseButton[i], i == 0);
        y += 10;
    }
    menuOptionsControlMouseButtonAssignment.Add(&itemBloodQAV, false);
}

void SetupMenus(void)
{
    // prepare gamefuncs and keys
    pzGamefuncsStrings[0] = MenuGameFuncNone;
    nGamefuncsValues[0] = -1;
    int k = 1;
    for (int i = 0; i < NUMGAMEFUNCTIONS; ++i)
    {
        Bstrcpy(MenuGameFuncs[i], gamefunctions[i]);

        for (int j = 0; j < MAXGAMEFUNCLEN; ++j)
            if (MenuGameFuncs[i][j] == '_')
                MenuGameFuncs[i][j] = ' ';

        if (gamefunctions[i][0] != '\0')
        {
            pzGamefuncsStrings[k] = MenuGameFuncs[i];
            nGamefuncsValues[k] = i;
            ++k;
        }
    }

    nGamefuncsNum = k;

    SetupLoadingScreen();
    SetupKeyListMenu();
    SetupMessagesMenu();
    SetupControlsMenu();
    SetupSaveGameMenu();
    SetupLoadGameMenu();
    SetupOptionsOldMenu();
    SetupCreditsMenu();
    SetupHelpOrderMenu();
    SetupSoundsMenu();
    SetupDifficultyMenu();
    SetupEpisodeMenu();
    SetupMainMenu();
    SetupMainMenuWithSave();
    SetupNetStartMenu();
    SetupQuitMenu();
    SetupParentalLockMenu();
    SetupSorryMenu();
    SetupSorry2Menu();
    SetupSorry3Menu();

    SetupOptionsMenu();
    SetupNetworkMenu();
}

void UpdateNetworkMenus(void)
{
    if (gGameOptions.nGameType > 0)
    {
        itemMain1.at24 = &menuNetStart;
        itemMain1.at28 = 2;
    }
    else
    {
        itemMain1.at24 = &menuEpisode;
        itemMain1.at28 = -1;
    }
    if (gGameOptions.nGameType > 0)
    {
        itemMainSave1.at24 = &menuNetStart;
        itemMainSave1.at28 = 2;
    }
    else
    {
        itemMainSave1.at24 = &menuEpisode;
        itemMainSave1.at28 = -1;
    }
}

void SetDoppler(CGameMenuItemZBool *pItem)
{
    gDoppler = pItem->at20;
}

void SetCrosshair(CGameMenuItemZBool *pItem)
{
    cl_crosshair = pItem->at20;
}

void SetCenterHoriz(CGameMenuItemZBool *pItem)
{
    gCenterHoriz = pItem->at20;
}

void ResetKeys(CGameMenuItemChain *)
{
    CONFIG_SetDefaultKeys(keydefaults);
}

void ResetKeysClassic(CGameMenuItemChain *)
{
    CONFIG_SetDefaultKeys(oldkeydefaults);
}

////
void SetWeaponsV10X(CGameMenuItemZBool* pItem)
{
    if (gGameOptions.nGameType == 0) {
        gWeaponsV10x = pItem->at20;
        gGameOptions.weaponsV10x = pItem->at20;
    }
}
////

void SetShowWeapons(CGameMenuItemZBool *pItem)
{
    gShowWeapon = pItem->at20;
}

void SetSlopeTilting(CGameMenuItemZBool *pItem)
{
    gSlopeTilting = pItem->at20;
}

void SetViewBobbing(CGameMenuItemZBool *pItem)
{
    gViewVBobbing = pItem->at20;
}

void SetViewSwaying(CGameMenuItemZBool *pItem)
{
    gViewHBobbing = pItem->at20;
}

void SetDetail(CGameMenuItemSlider *pItem)
{
    gDetail = pItem->nValue;
}

void SetGamma(CGameMenuItemSlider *pItem)
{
    gGamma = pItem->nValue;
    scrSetGamma(gGamma);
}

void SetMusicVol(CGameMenuItemSlider *pItem)
{
    sndSetMusicVolume(pItem->nValue);
}

void SetSoundVol(CGameMenuItemSlider *pItem)
{
    sndSetFXVolume(pItem->nValue);
}

void SetCDVol(CGameMenuItemSlider *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
    // NUKE-TODO:
}

void SetMessages(CGameMenuItemZBool *pItem)
{
    gMessageState = pItem->at20;
    gGameMessageMgr.SetState(gMessageState);
}

void SetMouseSensitivity(CGameMenuItemSliderFloat *pItem)
{
	CONTROL_MouseSensitivity = pItem->fValue;
}

void SetMouseAimFlipped(CGameMenuItemZBool *pItem)
{
    gMouseAimingFlipped = pItem->at20;
}

void SetTurnSpeed(CGameMenuItemSlider *pItem)
{
    gTurnSpeed = pItem->nValue;
}

void SetAutoAim(CGameMenuItemZCycle *pItem)
{
    gAutoAim = pItem->m_nFocus;
    if (!gDemo.at0 && !gDemo.at1)
    {
        gProfile[myconnectindex].nAutoAim = gAutoAim;
        netBroadcastPlayerInfo(myconnectindex);
    }
}

void SetLevelStats(CGameMenuItemZBool *pItem)
{
    gLevelStats = pItem->at20;
}

void SetPowerupDuration(CGameMenuItemZBool* pItem)
{
    gPowerupDuration = pItem->at20;
}

void SetShowMapTitle(CGameMenuItemZBool* pItem)
{
    gShowMapTitle = pItem->at20;
}

void SetWeaponSwitch(CGameMenuItemZCycle *pItem)
{
    gWeaponSwitch &= ~(1|2);
    switch (pItem->m_nFocus)
    {
    case 0:
        break;
    case 1:
        gWeaponSwitch |= 2;
        fallthrough__;
    case 2:
    default:
        gWeaponSwitch |= 1;
        break;
    }
    if (!gDemo.at0 && !gDemo.at1)
    {
        gProfile[myconnectindex].nWeaponSwitch = gWeaponSwitch;
        netBroadcastPlayerInfo(myconnectindex);
    }
}

extern bool gStartNewGame;

void SetDifficultyAndStart(CGameMenuItemChain *pItem)
{
    gGameOptions.nDifficulty = pItem->at30;
    gSkill = pItem->at30;
    gGameOptions.nLevel = 0;
    if (gDemo.at1)
        gDemo.StopPlayback();
    gStartNewGame = true;
    gCheatMgr.sub_5BCF4();
    gGameMenuMgr.Deactivate();
}

void SetVideoModeOld(CGameMenuItemChain *pItem)
{
    if (pItem->at30 == validmodecnt)
    {
        gSetup.fullscreen = 0;
        gSetup.xdim = 640;
        gSetup.ydim = 480;
    }
    else
    {
        gSetup.fullscreen = 0;
        gSetup.xdim = validmode[pItem->at30].xdim;
        gSetup.ydim = validmode[pItem->at30].ydim;
    }
    scrSetGameMode(gSetup.fullscreen, gSetup.xdim, gSetup.ydim, gSetup.bpp);
    scrSetDac();
    viewResizeView(gViewSize);
}

void SetVideoMode(CGameMenuItemChain *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
    resolution_t p = { xres, yres, fullscreen, bpp, 0 };
    int32_t prend = videoGetRenderMode();
    int32_t pvsync = vsync;

    int32_t nResolution = itemOptionsDisplayModeResolution.m_nFocus;
    resolution_t n = { gResolution[nResolution].xdim, gResolution[nResolution].ydim,
                       (gResolution[nResolution].flags & RES_FS) ? itemOptionsDisplayModeFullscreen.at20 : 0,
                       (nRendererValues[itemOptionsDisplayModeRenderer.m_nFocus] == REND_CLASSIC) ? 8 : gResolution[nResolution].bppmax, 0 };
    int32_t UNUSED(nrend) = nRendererValues[itemOptionsDisplayModeRenderer.m_nFocus];
    int32_t nvsync = nVSyncValues[itemOptionsDisplayModeVSync.m_nFocus];

    if (videoSetGameMode(n.flags, n.xdim, n.ydim, n.bppmax, upscalefactor) < 0)
    {
        if (videoSetGameMode(p.flags, p.xdim, p.ydim, p.bppmax, upscalefactor) < 0)
        {
            videoSetRenderMode(prend);
            ThrowError("Failed restoring old video mode.");
        }
        else
        {
            onvideomodechange(p.bppmax > 8);
            vsync = videoSetVsync(pvsync);
        }
    }
    else onvideomodechange(n.bppmax > 8);

    viewResizeView(gViewSize);
    vsync = videoSetVsync(nvsync);
    gSetup.fullscreen = fullscreen;
    gSetup.xdim = xres;
    gSetup.ydim = yres;
    gSetup.bpp = bpp;
}

void SetWidescreen(CGameMenuItemZBool *pItem)
{
    r_usenewaspect = pItem->at20;
}

void SetFOV(CGameMenuItemSlider *pItem)
{
    gFov = pItem->nValue;
}

void SetupVideoModeMenu(CGameMenuItemChain *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
    for (int i = 0; i < gResolutionNum; i++)
    {
        if (gSetup.xdim == gResolution[i].xdim && gSetup.ydim == gResolution[i].ydim)
        {
            itemOptionsDisplayModeResolution.m_nFocus = i;
            break;
        }
    }
    itemOptionsDisplayModeFullscreen.at20 = gSetup.fullscreen;
#ifdef USE_OPENGL
    for (int i = 0; i < 2; i++)
    {
        if (videoGetRenderMode() == nRendererValues[i])
        {
            itemOptionsDisplayModeRenderer.m_nFocus = i;
            break;
        }
    }
#endif
    for (int i = 0; i < 3; i++)
    {
        if (vsync == nVSyncValues[i])
        {
            itemOptionsDisplayModeVSync.m_nFocus = i;
            break;
        }
    }
    for (int i = 0; i < 8; i++)
    {
        if (r_maxfps == nFrameLimitValues[i])
        {
            itemOptionsDisplayModeFrameLimit.m_nFocus = i;
            break;
        }
    }
    itemOptionsDisplayModeFPSOffset.nValue = r_maxfpsoffset;
}

void PreDrawVideoModeMenu(CGameMenuItem *pItem)
{
    if (pItem == &itemOptionsDisplayModeFullscreen)
        pItem->bEnable = !!(gResolution[itemOptionsDisplayModeResolution.m_nFocus].flags & RES_FS);
#ifdef USE_OPENGL
    else if (pItem == &itemOptionsDisplayModeRenderer)
        pItem->bEnable = gResolution[itemOptionsDisplayModeResolution.m_nFocus].bppmax > 8;
#endif
}

void UpdateVideoModeMenuFrameLimit(CGameMenuItemZCycle *pItem)
{
    r_maxfps = nFrameLimitValues[pItem->m_nFocus];
    g_frameDelay = calcFrameDelay(r_maxfps + r_maxfpsoffset);
}

void UpdateVideoModeMenuFPSOffset(CGameMenuItemSlider *pItem)
{
    r_maxfpsoffset = pItem->nValue;
    g_frameDelay = calcFrameDelay(r_maxfps + r_maxfpsoffset);
}

void UpdateVideoColorMenu(CGameMenuItemSliderFloat *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
    g_videoGamma = itemOptionsDisplayColorGamma.fValue;
    g_videoContrast = itemOptionsDisplayColorContrast.fValue;
    g_videoBrightness = itemOptionsDisplayColorBrightness.fValue;
    r_ambientlight = itemOptionsDisplayColorVisibility.fValue;
    r_ambientlightrecip = 1.f/r_ambientlight;
    gBrightness = GAMMA_CALC<<2;
    videoSetPalette(gBrightness>>2, gLastPal, 0);
}

void PreDrawDisplayColor(CGameMenuItem *pItem)
{
    if (pItem == &itemOptionsDisplayColorContrast)
        pItem->bEnable = 1;
    else if (pItem == &itemOptionsDisplayColorBrightness)
        pItem->bEnable = 1;
}

void ResetVideoColor(CGameMenuItemChain *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
    g_videoGamma = DEFAULT_GAMMA;
    g_videoContrast = DEFAULT_CONTRAST;
    g_videoBrightness = DEFAULT_BRIGHTNESS;
    gBrightness = 0;
    r_ambientlight = r_ambientlightrecip = 1.f;
    videoSetPalette(gBrightness>>2, gLastPal, 0);
}

#ifdef USE_OPENGL
void SetupVideoPolymostMenu(CGameMenuItemChain *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
    itemOptionsDisplayPolymostTextureMode.m_nFocus = 0;
    for (int i = 0; i < 2; i++)
    {
        if (nTextureModeValues[i] == gltexfiltermode)
        {
            itemOptionsDisplayPolymostTextureMode.m_nFocus = i;
            break;
        }
    }
    itemOptionsDisplayPolymostAnisotropy.m_nFocus = 0;
    for (int i = 0; i < 6; i++)
    {
        if (nAnisotropyValues[i] == glanisotropy)
        {
            itemOptionsDisplayPolymostAnisotropy.m_nFocus = i;
            break;
        }
    }
    itemOptionsDisplayPolymostTrueColorTextures.at20 = usehightile;
    itemOptionsDisplayPolymostTexQuality.m_nFocus = r_downsize;
    itemOptionsDisplayPolymostPreloadCache.at20 = useprecache;
    itemOptionsDisplayPolymostDetailTex.at20 = r_detailmapping;
    itemOptionsDisplayPolymostGlowTex.at20 = r_glowmapping;
    itemOptionsDisplayPolymost3DModels.at20 = usemodels;
    itemOptionsDisplayPolymostDeliriumBlur.at20 = gDeliriumBlur;
}

void UpdateTextureMode(CGameMenuItemZCycle *pItem)
{
    gltexfiltermode = nTextureModeValues[pItem->m_nFocus];
    gltexapplyprops();
}

void UpdateAnisotropy(CGameMenuItemZCycle *pItem)
{
    glanisotropy = nAnisotropyValues[pItem->m_nFocus];
    gltexapplyprops();
}

void UpdateTrueColorTextures(CGameMenuItemZBool *pItem)
{
    usehightile = pItem->at20;
}
#endif

void DoModeChange(void)
{
    videoResetMode();
    if (videoSetGameMode(fullscreen, xres, yres, bpp, upscalefactor))
        OSD_Printf("restartvid: Reset failed...\n");
    onvideomodechange(gSetup.bpp > 8);
}

#ifdef USE_OPENGL
void UpdateTexQuality(CGameMenuItemZCycle *pItem)
{
    r_downsize = pItem->m_nFocus;
    r_downsizevar = r_downsize;
    DoModeChange();
}

void UpdatePreloadCache(CGameMenuItemZBool *pItem)
{
    useprecache = pItem->at20;
}

void UpdateDetailTex(CGameMenuItemZBool *pItem)
{
    r_detailmapping = pItem->at20;
}

void UpdateGlowTex(CGameMenuItemZBool *pItem)
{
    r_glowmapping = pItem->at20;
}

void Update3DModels(CGameMenuItemZBool *pItem)
{
    usemodels = pItem->at20;
}

void UpdateDeliriumBlur(CGameMenuItemZBool *pItem)
{
    gDeliriumBlur = pItem->at20;
}

void PreDrawDisplayPolymost(CGameMenuItem *pItem)
{
    if (pItem == &itemOptionsDisplayPolymostTexQuality)
        pItem->bEnable = usehightile;
    else if (pItem == &itemOptionsDisplayPolymostPreloadCache)
        pItem->bEnable = usehightile;
    else if (pItem == &itemOptionsDisplayPolymostDetailTex)
        pItem->bEnable = usehightile;
    else if (pItem == &itemOptionsDisplayPolymostGlowTex)
        pItem->bEnable = usehightile;
}
#endif

void UpdateSoundToggle(CGameMenuItemZBool *pItem)
{
    SoundToggle = pItem->at20;
    if (!SoundToggle)
        FX_StopAllSounds();
}

void UpdateMusicToggle(CGameMenuItemZBool *pItem)
{
    MusicToggle = pItem->at20;
    if (!MusicToggle)
        sndStopSong();
    else
    {
        if (gGameStarted || gDemo.at1)
            sndPlaySong(gGameOptions.zLevelSong, true);
    }
}

void Update3DToggle(CGameMenuItemZBool *pItem)
{
    gDoppler = pItem->at20;
}

void UpdateCDToggle(CGameMenuItemZBool *pItem)
{
    CDAudioToggle = pItem->at20;
    if (gGameStarted || gDemo.at1)
        levelTryPlayMusicOrNothing(gGameOptions.nEpisode, gGameOptions.nLevel);
}

void UpdateSoundVolume(CGameMenuItemSlider *pItem)
{
    sndSetFXVolume(pItem->nValue);
}

void UpdateMusicVolume(CGameMenuItemSlider *pItem)
{
    sndSetMusicVolume(pItem->nValue);
}

void UpdateSoundRate(CGameMenuItemZCycle *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
}

void UpdateNumVoices(CGameMenuItemSlider *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
}

void UpdateMusicDevice(CGameMenuItemZCycle *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
}

void SetSound(CGameMenuItemChain *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
    MixRate = nSoundRateValues[itemOptionsSoundSampleRate.m_nFocus];
    NumVoices = itemOptionsSoundNumVoices.nValue;
    MusicDevice = itemOptionsSoundMusicDevice.m_nFocus;
    sfxTerm();
    sndTerm();

    sndInit();
    sfxInit();

    if (MusicToggle && (gGameStarted || gDemo.at1))
        sndPlaySong(gGameOptions.zLevelSong, true);
}

void PreDrawSound(CGameMenuItem *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
}

void SetupOptionsSound(CGameMenuItemChain *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
    itemOptionsSoundSoundToggle.at20 = SoundToggle;
    itemOptionsSoundMusicToggle.at20 = MusicToggle;
    itemOptionsSound3DToggle.at20 = gDoppler;
    itemOptionsSoundCDToggle.at20 = CDAudioToggle;
    itemOptionsSoundSampleRate.m_nFocus = 0;
    for (int i = 0; i < 3; i++)
    {
        if (nSoundRateValues[i] == MixRate)
        {
            itemOptionsSoundSampleRate.m_nFocus = i;
            break;
        }
    }
    itemOptionsSoundNumVoices.nValue = NumVoices;
    itemOptionsSoundMusicDevice.m_nFocus = MusicDevice;
}

void UpdatePlayerName(CGameMenuItemZEdit *pItem, CGameMenuEvent *pEvent)
{
    UNREFERENCED_PARAMETER(pItem);
    if (pEvent->at0 == kMenuEventEnter)
        netBroadcastPlayerInfo(myconnectindex);
}

void SetMouseFilterInput(CGameMenuItemZBool *pItem)
{
    CONTROL_SmoothMouse = pItem->at20;
    SmoothInput = pItem->at20;
}

void SetMouseAimMode(CGameMenuItemZBool *pItem)
{
    gMouseAiming = pItem->at20;
}

void SetMouseVerticalAim(CGameMenuItemZBool *pItem)
{
    gMouseAim = pItem->at20;
}

void SetMouseXScale(CGameMenuItemSlider *pItem)
{
    MouseAnalogueScale[0] = pItem->nValue;
    CONTROL_SetAnalogAxisScale(0, pItem->nValue, controldevice_mouse);
}

void SetMouseYScale(CGameMenuItemSlider *pItem)
{
    MouseAnalogueScale[1] = pItem->nValue;
    CONTROL_SetAnalogAxisScale(1, pItem->nValue, controldevice_mouse);
}

void SetMouseDigitalAxis(CGameMenuItemZCycle *pItem)
{
    if (pItem == &itemOptionsControlMouseDigitalUp)
    {
        MouseDigitalFunctions[1][0] = nGamefuncsValues[pItem->m_nFocus];
        CONTROL_MapDigitalAxis(1, MouseDigitalFunctions[1][0], 0, controldevice_mouse);
    }
    else if (pItem == &itemOptionsControlMouseDigitalDown)
    {
        MouseDigitalFunctions[1][1] = nGamefuncsValues[pItem->m_nFocus];
        CONTROL_MapDigitalAxis(1, MouseDigitalFunctions[1][1], 1, controldevice_mouse);
    }
    else if (pItem == &itemOptionsControlMouseDigitalLeft)
    {
        MouseDigitalFunctions[0][0] = nGamefuncsValues[pItem->m_nFocus];
        CONTROL_MapDigitalAxis(0, MouseDigitalFunctions[0][0], 0, controldevice_mouse);
    }
    else if (pItem == &itemOptionsControlMouseDigitalRight)
    {
        MouseDigitalFunctions[0][1] = nGamefuncsValues[pItem->m_nFocus];
        CONTROL_MapDigitalAxis(0, MouseDigitalFunctions[0][1], 1, controldevice_mouse);
    }
}

void SetupMouseMenu(CGameMenuItemChain *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
    static CGameMenuItemZCycle *pMouseDigitalAxis[4] = {
        &itemOptionsControlMouseDigitalLeft,
        &itemOptionsControlMouseDigitalRight,
        &itemOptionsControlMouseDigitalUp,
        &itemOptionsControlMouseDigitalDown
    };
    for (int i = 0; i < ARRAY_SSIZE(pMouseDigitalAxis); i++)
    {
        CGameMenuItemZCycle *pItem = pMouseDigitalAxis[i];
        pItem->m_nFocus = 0;
        for (int j = 0; j < NUMGAMEFUNCTIONS+1; j++)
        {
            if (nGamefuncsValues[j] == MouseDigitalFunctions[i>>1][i&1])
            {
                pItem->m_nFocus = j;
                break;
            }
        }
    }
    itemOptionsControlMouseAimFlipped.at20 = gMouseAimingFlipped;
    itemOptionsControlMouseFilterInput.at20 = SmoothInput;
    itemOptionsControlMouseAimMode.at20 = gMouseAiming;
    itemOptionsControlMouseVerticalAim.at20 = gMouseAim;
}

void PreDrawControlMouse(CGameMenuItem *pItem)
{
    if (pItem == &itemOptionsControlMouseVerticalAim)
        pItem->bEnable = !gMouseAiming;
}

void SetMouseButton(CGameMenuItemZCycle *pItem)
{
    for (int i = 0; i < MENUMOUSEFUNCTIONS; i++)
    {
        if (pItem == pItemOptionsControlMouseButton[i])
        {
            int nFunc = nGamefuncsValues[pItem->m_nFocus];
            MouseFunctions[MenuMouseDataIndex[i][0]][MenuMouseDataIndex[i][1]] = nFunc;
            CONTROL_MapButton(nFunc, MenuMouseDataIndex[i][0], MenuMouseDataIndex[i][1], controldevice_mouse);
            CONTROL_FreeMouseBind(MenuMouseDataIndex[i][0]);
            break;
        }
    }
}

void SetupMouseButtonMenu(CGameMenuItemChain *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
    for (int i = 0; i < MENUMOUSEFUNCTIONS; i++)
    {
        auto pItem = pItemOptionsControlMouseButton[i];
        pItem->m_nFocus = 0;
        for (int j = 0; j < NUMGAMEFUNCTIONS+1; j++)
        {
            if (MouseFunctions[MenuMouseDataIndex[i][0]][MenuMouseDataIndex[i][1]] == nGamefuncsValues[j])
            {
                pItem->m_nFocus = j;
                break;
            }
        }
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
    sndStopSong();
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
    sndStopSong();
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

void SaveGameProcess(CGameMenuItemChain *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
}

void TenProcess(CGameMenuItem7EA1C *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
}

short gQuickLoadSlot = -1;
short gQuickSaveSlot = -1;

void SaveGame(CGameMenuItemZEditBitmap *pItem, CGameMenuEvent *event)
{
    char strSaveGameName[BMAX_PATH];
    int nSlot = pItem->at28;
    if (gGameOptions.nGameType > 0 || !gGameStarted)
        return;
    if (event->at0 != 6/* || strSaveGameName[0]*/)
    {
        gGameMenuMgr.Deactivate();
        return;
    }
    G_ModDirSnprintf(strSaveGameName, BMAX_PATH, "game00%02d.sav", nSlot);
    strcpy(gGameOptions.szUserGameName, strRestoreGameStrings[nSlot]);
    sprintf(gGameOptions.szSaveGameName, "%s", strSaveGameName);
    gGameOptions.nSaveGameSlot = nSlot;
    viewLoadingScreen(2518, "Saving", "Saving Your Game", strRestoreGameStrings[nSlot]);
    videoNextPage();
    gSaveGameNum = nSlot;
    LoadSave::SaveGame(strSaveGameName);
    gQuickSaveSlot = nSlot;
    gGameMenuMgr.Deactivate();
}

void QuickSaveGame(void)
{
    char strSaveGameName[BMAX_PATH];
    if (gGameOptions.nGameType > 0 || !gGameStarted)
        return;
    /*if (strSaveGameName[0])
    {
        gGameMenuMgr.Deactivate();
        return;
    }*/
    G_ModDirSnprintf(strSaveGameName, BMAX_PATH, "game00%02d.sav", gQuickSaveSlot);
    strcpy(gGameOptions.szUserGameName, strRestoreGameStrings[gQuickSaveSlot]);
    sprintf(gGameOptions.szSaveGameName, "%s", strSaveGameName);
    gGameOptions.nSaveGameSlot = gQuickSaveSlot;
    viewLoadingScreen(2518, "Saving", "Saving Your Game", strRestoreGameStrings[gQuickSaveSlot]);
    videoNextPage();
    LoadSave::SaveGame(strSaveGameName);
    gGameOptions.picEntry = gSavedOffset;
    gSaveGameOptions[gQuickSaveSlot] = gGameOptions;
    UpdateSavedInfo(gQuickSaveSlot);
    gGameMenuMgr.Deactivate();
}

void LoadGame(CGameMenuItemZEditBitmap *pItem, CGameMenuEvent *event)
{
    UNREFERENCED_PARAMETER(event);
    char strLoadGameName[BMAX_PATH];
    int nSlot = pItem->at28;
    if (gGameOptions.nGameType > 0)
        return;
    G_ModDirSnprintf(strLoadGameName, BMAX_PATH, "game00%02d.sav", nSlot);
    if (!testkopen(strLoadGameName, 0))
        return;
    viewLoadingScreen(2518, "Loading", "Loading Saved Game", strRestoreGameStrings[nSlot]);
    videoNextPage();
    LoadSave::LoadGame(strLoadGameName);
    gGameMenuMgr.Deactivate();
    gQuickLoadSlot = nSlot;
}

void QuickLoadGame(void)
{
    char strLoadGameName[BMAX_PATH];
    if (gGameOptions.nGameType > 0)
        return;
    G_ModDirSnprintf(strLoadGameName, BMAX_PATH, "game00%02d.sav", gQuickLoadSlot);
    if (!testkopen(strLoadGameName, 0))
        return;
    viewLoadingScreen(2518, "Loading", "Loading Saved Game", strRestoreGameStrings[gQuickLoadSlot]);
    videoNextPage();
    LoadSave::LoadGame(strLoadGameName);
    gGameMenuMgr.Deactivate();
}

void SetupLevelMenuItem(int nEpisode)
{
    dassert(nEpisode >= 0 && nEpisode < gEpisodeCount);
    itemNetStart3.SetTextArray(zLevelNames[nEpisode], gEpisodeInfo[nEpisode].nLevels, 0);
}

void SetupNetLevels(CGameMenuItemZCycle *pItem)
{
    SetupLevelMenuItem(pItem->m_nFocus);
}

void StartNetGame(CGameMenuItemChain *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
    gPacketStartGame.gameType = itemNetStart1.m_nFocus+1;
    if (gPacketStartGame.gameType == 0)
        gPacketStartGame.gameType = 2;
    gPacketStartGame.episodeId = itemNetStart2.m_nFocus;
    gPacketStartGame.levelId = itemNetStart3.m_nFocus;
    gPacketStartGame.difficulty = itemNetStart4.m_nFocus;
    gPacketStartGame.monsterSettings = itemNetStart5.m_nFocus;
    gPacketStartGame.weaponSettings = itemNetStart6.m_nFocus;
    gPacketStartGame.itemSettings = itemNetStart7.m_nFocus;
    gPacketStartGame.respawnSettings = 0;
    gPacketStartGame.bFriendlyFire = itemNetStart8.at20;
    gPacketStartGame.bKeepKeysOnRespawn = itemNetStart9.at20;
    ////
    gPacketStartGame.weaponsV10x = itemNetStart10.at20;
    ////
    gPacketStartGame.unk = 0;
    gPacketStartGame.userMapName[0] = 0;
    strncpy(gPacketStartGame.userMapName, itemNetStart11.at20, 13);
    gPacketStartGame.userMapName[12] = 0;
    gPacketStartGame.userMap = gPacketStartGame.userMapName[0] != 0;

    netBroadcastNewGame();
    gStartNewGame = 1;
    gGameMenuMgr.Deactivate();
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

void Quit(CGameMenuItemChain *pItem)
{
    UNREFERENCED_PARAMETER(pItem);
    if (gGameOptions.nGameType == 0 || numplayers == 1)
        gQuitGame = true;
    else
        gQuitRequest = 1;
    gGameMenuMgr.Deactivate();
}

void SetParentalLock(CGameMenuItemZBool *pItem)
{
    if (!pItem->at20)
    {
        pItem->at20 = true;
        pItem->Draw();
        if (strcmp(itemParentalLockPassword.at20, ""))
        {
            itemParentalLockPassword.pMenu->FocusNextItem();
            itemParentalLockPassword.at32 = 0;
            itemParentalLockPassword.at37 = 1;
            itemParentalLockPassword.at5f = pItem;
            itemParentalLockPassword.at29[0] = 0;
            return;
        }
        else
        {
            itemParentalLockPassword.at20[0] = 0;
            pItem->Draw();
            gbAdultContent = false;
        }
    }
    else
        gbAdultContent = true;
    // NUKE-TODO: CONFIG_WriteAdultMode();
}

void MenuSetupEpisodeInfo(void)
{
    memset(zEpisodeNames, 0, sizeof(zEpisodeNames));
    memset(zLevelNames, 0, sizeof(zLevelNames));
    for (int i = 0; i < 6; i++)
    {
        if (i < gEpisodeCount)
        {
            EPISODEINFO *pEpisode = &gEpisodeInfo[i];
            zEpisodeNames[i] = pEpisode->at0;
            for (int j = 0; j < 16; j++)
            {
                if (j < pEpisode->nLevels)
                {
                    zLevelNames[i][j] = pEpisode->at28[j].at90;
                }
            }
        }
    }
}

void drawLoadingScreen(void)
{
    char buffer[80];
    if (gGameOptions.nGameType == 0)
    {
        if (gDemo.at1)
            sprintf(buffer, "Loading Demo");
        else
            sprintf(buffer, "Loading Level");
    }
    else
        sprintf(buffer, "%s", zNetGameTypes[gGameOptions.nGameType-1]);
    viewLoadingScreen(2049, buffer, levelGetTitle(), NULL);
}

END_BLD_NS
