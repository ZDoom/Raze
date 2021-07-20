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

#pragma once

#include "compat.h"
#include "v_text.h"
#include "printf.h"
#include "gamecvars.h"
#include "m_argv.h"
#include "gamecontrol.h"
#include "c_buttons.h"
#include <algorithm>
#include "tarray.h"
#include "zstring.h"
#include "filesystem.h"
#include "screenjob.h"
#include "gamestruct.h"
#include "names.h"

BEGIN_PS_NS


enum { kTimerTicks = 120 };

enum basepal_t {
    BASEPAL = 0,
    ANIMPAL,
    BASEPALCOUNT
};

void DebugOut(const char *fmt, ...);
int ExhumedMain(int argc, char *argv[]);

void ResetEngine();

void SetHiRes();

void BlackOut();

void DoGameOverScene(bool finallevel);

int Query(short n, short l, ...);

extern unsigned char curpal[];

void TintPalette(int a, int b, int c);
//void MySetPalette(unsigned char *palette);
//void GetCurPal(unsigned char *palette);

void EraseScreen(int eax);

void mychangespritesect(int nSprite, int nSector);
void mydeletesprite(int nSprite);

void GrabPalette();

void StartFadeIn();

void InitSpiritHead();

// TODO - relocate
void StatusMessage(int messageTime, const char *fmt, ...);

void DoSpiritHead();

void CheckKeys2();
void GameTicker();
void InitLevel(MapRecord*);
void InitNewGame();

int showmap(short nLevel, short nLevelNew, short nLevelBest);
void menu_DoPlasma();
void DoEnergyTile();
void InitEnergyTile();

extern int EndLevel;
extern int32_t g_commandSetup;
extern int32_t g_noSetup;

extern char sHollyStr[];
extern int selectedlevelnew;

extern int nNetPlayerCount;

extern int nNetTime;

extern short nTotalPlayers;

extern short nFontFirstChar;
extern short nBackgroundPic;
extern short nShadowPic;

extern short nCreaturesTotal, nCreaturesKilled;

extern int lLocalButtons;

extern short nEnergyTowers;

extern short nEnergyChan;

extern short nSpiritSprite;

extern short bInDemo;

extern short nFreeze;

extern short nCurBodyNum;
extern short nBodyTotal;

extern short bSnakeCam;

extern short nButtonColor;

extern short nHeadStage;

extern short lastfps;

extern int flash;

extern short nLocalSpr;

extern short nSnakeCam;

extern short bCoordinates;

extern int totalmoves;

extern int lCountDown;
extern short nAlarmTicks;
extern short nRedTicks;
extern short nClockVal;

extern short bSlipMode;
extern short bDoFlashes;

extern int bVanilla;

inline int GameLogo()
{
    return (g_gameType & GAMEFLAG_EXHUMED) ? kExhumedLogo : kPowerslaveLogo;
}

extern double g_frameDelay;

enum {
    kPalNormal = 0,
    kPalNoDim,
    kPalTorch,
    kPalNoTorch,
    kPalBrite,
    kPalRedBrite,
    kPalGreenBrite,
    kPalNormal2,
    kPalNoDim2,
    kPalTorch2,
    kPalNoTorch2,
    kPalBrite2
};


class TextOverlay
{
    FFont* font;
	double nCrawlY;
	short nLeft[50];
	int nHeight;
    double lastclock;
	TArray<FString> screentext;
    int currentCinemaPalette = 0;


public:

	void Start(double starttime);
	void ComputeCinemaText();
	void ReadyCinemaText(const char* nVal);
	void DisplayText();
	bool AdvanceCinemaText(double clock);
    void SetPalette(int pal) { currentCinemaPalette = pal; }
    void Create(const FString& text, int pal);
};


extern char g_modDir[BMAX_PATH];

void G_LoadGroupsInDir(const char* dirname);
void G_DoAutoload(const char* dirname);
void DrawRel(int tile, double x, double y, int shade = 0);
void LevelFinished();

// savegame.

int savegame(int nSlot);
int loadgame(int nSlot);

const uint32_t kSpiritX = 106;
const uint32_t kSpiritY = 97;
const uint32_t WorktileSize = kSpiritX * 2 * kSpiritY * 2;


struct GameInterface : public ::GameInterface
{
    const char* Name() override { return "Exhumed"; }
    void app_init() override;
    void clearlocalinputstate() override;
    void loadPalette() override;
	bool GenerateSavePic() override;
    void MenuOpened() override;
    void MenuSound(EMenuSounds snd) override;
    FSavegameInfo GetSaveSig() override;
    void SerializeGameState(FSerializer& arc);
    bool CanSave() override;
    ReservedSpace GetReservedScreenSpace(int viewsize) override { return { 0, 24 }; }
    void UpdateSounds() override;
    void ErrorCleanup() override;
    void Ticker() override;
    void DrawBackground() override;
    void Render() override;
    void GetInput(InputPacket* packet, ControlInfo* const hidInput) override;
    void Startup() override;
    const char* GenericCheat(int player, int cheat) override;
	void NewGame(MapRecord *map, int skill, bool) override;
	void LevelCompleted(MapRecord *map, int skill) override;
	void NextLevel(MapRecord *map, int skill) override;
    bool DrawAutomapPlayer(int x, int y, int z, int a, double const smoothratio) override;
    fixed_t playerHorizMin() override { return IntToFixed(-150); }
    fixed_t playerHorizMax() override { return IntToFixed(150); }
    int playerKeyMove() override { return 6; }
    void WarpToCoords(int x, int y, int z, int a, int h) override;
    void ToggleThirdPerson() override;
    int chaseCamX(binangle ang) { return -ang.bcos() / 12; }
    int chaseCamY(binangle ang) { return -ang.bsin() / 12; }
    int chaseCamZ(fixedhoriz horiz) { return horiz.asq16() / 384; }
    void processSprites(spritetype* tsprite, int& spritesortcnt, int viewx, int viewy, int viewz, binangle viewang, double smoothRatio) override;
    int GetCurrentSkill() override;

	::GameStats getStats() override;
};


END_PS_NS

