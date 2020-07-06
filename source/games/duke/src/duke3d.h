#ifndef duke3d_h_
#define duke3d_h_

// JBF
#include "baselayer.h"
#include "build.h"

#include "compat.h"

#include "pragmas.h"

#include "polymost.h"
#include "gamecvars.h"
#include "menu/menu.h"
#include "zz_actors.h"
#include "gamecontrol.h"
#include "game.h"
#include "gamedef.h"
#include "gameexec.h"
#include "gamevar.h"
#include "global.h"
#include "macros.h"
#include "names.h"
#include "net.h"
#include "player.h"
#include "quotemgr.h"
#include "rts.h"
#include "sounds.h"
#include "soundefs.h"

BEGIN_DUKE_NS

extern FFont* IndexFont;
extern FFont* DigiFont;

struct GameInterface : ::GameInterface
{
	const char* Name() override { return "Duke"; }
	int app_main() override;
	void UpdateScreenSize() override;
	void FreeGameData() override;
	bool GenerateSavePic() override;
	bool validate_hud(int) override;
	void set_hud_layout(int size) override;
	void set_hud_scale(int size) override;
	FString statFPS() override;
	GameStats getStats() override;
	void DrawNativeMenuText(int fontnum, int state, double xpos, double ypos, float fontscale, const char* text, int flags) override;
	void MenuOpened() override;
	void MenuSound(EMenuSounds snd) override;
	void MenuClosed() override;
	bool CanSave() override;
	void StartGame(FNewGameStartup& gs) override;
	FSavegameInfo GetSaveSig() override;
	void DrawCenteredTextScreen(const DVector2& origin, const char* text, int position, bool bg) override;
	double SmallFontScale() override { return isRR() ? 0.5 : 1.; }
	void DrawMenuCaption(const DVector2& origin, const char* text) override;
	bool SaveGame(FSaveGameNode*) override;
	bool LoadGame(FSaveGameNode*) override;
	void QuitToTitle() override;
	FString GetCoordString() override;
	bool CheatAllowed(bool printmsg) override;
};

END_DUKE_NS

#endif
