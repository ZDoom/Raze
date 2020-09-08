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

#ifndef __exhumed_h__
#define __exhumed_h__

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

BEGIN_PS_NS


#define kTimerTicks		120

enum basepal_t {
    BASEPAL = 0,
    ANIMPAL,
    BASEPALCOUNT
};

void ExitGame();
void ShutDown(void);
void DebugOut(const char *fmt, ...);
int ExhumedMain(int argc, char *argv[]);

void ResetEngine();

void SetHiRes();

void BlackOut();

void DoGameOverScene(bool finallevel);
void DoAfterCinemaScene(int nLevel, TArray<JobDesc> &jobs);
void DoBeforeCinemaScene(int nLevel, TArray<JobDesc>& jobs);

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
void InitLevel(int);
void InitNewGame();

int showmap(short nLevel, short nLevelNew, short nLevelBest);
void menu_DoPlasma();
void menu_DrawTheMap(int nLevel, int nLevelNew, int nLevelBest, TArray<JobDesc>& jobs);
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
extern int leveltime;

extern int lLocalButtons;

extern short nEnergyTowers;

extern short nEnergyChan;

extern short nSpiritSprite;

extern short bInDemo;

extern short nFreeze;

extern short nCurBodyNum;
extern short nBodyTotal;

extern short bSnakeCam;
extern uint8_t nCinemaSeen;

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

#define POWERSLAVE  (g_gameType & GAMEFLAG_POWERSLAVE)
#define EXHUMED     (g_gameType & GAMEFLAG_EXHUMED)
#define ISDEMOVER   (g_gameType & GAMEFLAG_SHAREWARE)

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
	double nCrawlY;
	short nLeft[50];
	int nHeight;
    double lastclock;
	TArray<FString> screentext;
    int currentCinemaPalette = 0;


public:

	void Start(double starttime);
	void ComputeCinemaText();
	void ReadyCinemaText(uint16_t nVal);
	void DisplayText();
	bool AdvanceCinemaText(double clock);
    void SetPalette(int pal) { currentCinemaPalette = pal; }
};


extern char g_modDir[BMAX_PATH];

void G_LoadGroupsInDir(const char* dirname);
void G_DoAutoload(const char* dirname);
void DrawRel(int tile, double x, double y, int shade = 0);

// savegame.

int savegame(int nSlot);
int loadgame(int nSlot);

const uint32_t kSpiritX = 106;
const uint32_t kSpiritY = 97;
const uint32_t WorktileSize = kSpiritX * 2 * kSpiritY * 2;

struct SavegameHelper
{
    FString Name;
    TArray<std::pair<void*, size_t>> Elements;
    SavegameHelper(const char* name, ...);
    void Load();
    void Save();
};

#define SV(v) &v, sizeof(v)
#define SA(a) &a, sizeof(a)



struct GameInterface : ::GameInterface
{
    const char* Name() override { return "Exhumed"; }
    void app_init() override;
	bool GenerateSavePic() override;
    void DrawNativeMenuText(int fontnum, int state, double xpos, double ypos, float fontscale, const char* text, int flags) override;
    void MenuOpened() override;
    void MenuSound(EMenuSounds snd) override;
    void MenuClosed() override;
    void StartGame(FNewGameStartup& gs) override;
    FSavegameInfo GetSaveSig() override;
    void DrawMenuCaption(const DVector2& origin, const char* text) override;
    bool LoadGame(FSaveGameNode* sv) override;
    bool SaveGame(FSaveGameNode* sv) override;
    bool CanSave() override;
    ReservedSpace GetReservedScreenSpace(int viewsize) override { return { 0, 24 }; }
	void QuitToTitle() override;
    void UpdateSounds() override;
    void ErrorCleanup() override;
    void Ticker() override;
    void DrawBackground() override;
    void Render() override;
    void GetInput(InputPacket* packet) override;
    void Startup() override;
    const char* GenericCheat(int player, int cheat) override;
	void NewGame(MapRecord *map, int skill) override;
	void LevelCompleted(MapRecord *map, int skill) override;
	void NextLevel(MapRecord *map, int skill) override;
    bool DrawAutomapPlayer(int x, int y, int z, int a) override;


	::GameStats getStats() override;
};


END_PS_NS

#endif
