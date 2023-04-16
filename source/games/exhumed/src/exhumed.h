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
#include "exhumedactor.h"
#include "serialize_obj.h"
#include "texturemanager.h"
#include "player.h"

BEGIN_PS_NS


enum { kTimerTicks = 120 };

const int ITEM_MAGIC = 0x4711;

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

extern uint8_t curpal[];

void TintPalette(int a, int b, int c);

void EraseScreen(int eax);

void DeleteActor(DExhumedActor* actor);

void GrabPalette();

void StartFadeIn();

void InitSpiritHead();

// TODO - relocate
void StatusMessage(int messageTime, const char *fmt, ...);

void DoSpiritHead();

void InitLevel(MapRecord*);
void InitNewGame();

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

extern int nTotalPlayers;

extern int nCreaturesTotal, nCreaturesKilled;

extern int lLocalButtons;

extern int nEnergyTowers;

extern int nEnergyChan;

extern TObjPtr<DExhumedActor*> pSpiritSprite;

extern bool bInDemo;

extern int nFreeze;

extern int nCurBodyNum;
extern int nBodyTotal;

extern bool bSnakeCam;

extern int nButtonColor;

extern int nHeadStage;

extern int flash;

extern int nSnakeCam;

extern bool bCoordinates;

extern int totalmoves;

extern int lCountDown;
extern int nAlarmTicks;
extern int nRedTicks;
extern int nClockVal;

extern bool bSlipMode;
extern bool bDoFlashes;

extern int bVanilla;

inline FTextureID GameLogo()
{
    return TexMan.CheckForTexture((g_gameType & GAMEFLAG_EXHUMED) ? "ExhumedLogo" : "PowerslaveLogo", ETextureType::Any);
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
	int16_t nLeft[50];
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
    void SetupSpecialTextures(TilesetBuildInfo& info) override;
    void loadPalette() override;
	bool GenerateSavePic() override;
    void MenuOpened() override;
    void MenuSound(EMenuSounds snd) override;
    FSavegameInfo GetSaveSig() override;
    void SerializeGameState(FSerializer& arc);
    bool CanSave() override;
    void UpdateSounds() override;
    void ErrorCleanup() override;
    void Ticker() override;
    void DrawBackground() override;
    void Render() override;
    //void DrawWeapons() override;
    void Startup() override;
    const char* GenericCheat(int player, int cheat) override;
	void NewGame(MapRecord *map, int skill, bool) override;
	void LevelCompleted(MapRecord *map, int skill) override;
	void NextLevel(MapRecord *map, int skill) override;
    bool DrawAutomapPlayer(const DVector2& mxy, const DVector2& cpos, const DAngle cang, const DVector2& xydim, const double czoom, double const interpfrac) override;
    DAngle playerPitchMin() override { return DAngle::fromDeg(49.5); }
    DAngle playerPitchMax() override { return DAngle::fromDeg(-49.5); }
    DCoreActor* getConsoleActor() override { return PlayerList[nLocalPlayer].pActor; }
    void ToggleThirdPerson() override;
    void processSprites(tspriteArray& tsprites, const DVector3& view, DAngle viewang, double interpfrac) override;
    int GetCurrentSkill() override;
    void StartSoundEngine() override;
    void reapplyInputBits(InputPacket* const input) override { input->actions |= PlayerList[nLocalPlayer].input.actions & SB_CENTERVIEW; }
    void doPlayerMovement(const float scaleAdjust) override { gameInput.processMovement(&PlayerList[nLocalPlayer].Angles, scaleAdjust); }

	::GameStats getStats() override;
};


END_PS_NS

