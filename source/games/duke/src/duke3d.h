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
#include "funct.h"
#include "gamecontrol.h"
#include "gamevar.h"
#include "global.h"
#include "names.h"
#include "quotemgr.h"
#include "rts.h"
#include "sounds.h"
#include "soundefs.h"
#include "stats.h"
#include "binaryangle.h"

extern glcycle_t drawtime, actortime, thinktime, gameupdatetime;

BEGIN_DUKE_NS

extern FFont* IndexFont;
extern FFont* DigiFont;

struct GameInterface : ::GameInterface
{
	const char* Name() override { return "Duke"; }
	int app_main() override;
	void clearlocalinputstate() override;
	void UpdateScreenSize() override;
	bool GenerateSavePic() override;
	void set_hud_layout(int size) override;
	void PlayHudSound() override;
	bool automapActive() override;
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
	void SerializeGameState(FSerializer& arc) override;
	void QuitToTitle() override;
	FString GetCoordString() override;
	bool CheatAllowed(bool printmsg) override;
	void ExitFromMenu() override;
};

struct Dispatcher
{
	// global stuff
	void (*ShowLogo)(const CompletionFunc& completion);
	void (*InitFonts)();
	void (*PrintPaused)();

	// sectors_?.cpp
	void (*think)();
	void (*initactorflags)();
	bool (*isadoorwall)(int dapic);
	void (*animatewalls)();
	void (*operaterespawns)(int low);
	void (*operateforcefields)(int s, int low);
	bool (*checkhitswitch)(int snum, int w, int switchtype);
	void (*activatebysector)(int sect, int j);
	void (*checkhitwall)(int spr, int dawallnum, int x, int y, int z, int atwith);
	void (*checkplayerhurt)(struct player_struct* p, int j);
	bool (*checkhitceiling)(int sn);
	void (*checkhitsprite)(int i, int sn);
	void (*checksectors)(int low);

	bool (*ceilingspace)(int sectnum);
	bool (*floorspace)(int sectnum);
	void (*addweapon)(struct player_struct *p, int weapon);
	void (*hitradius)(short i, int  r, int  hp1, int  hp2, int  hp3, int  hp4);
	int  (*movesprite)(short spritenum, int xchange, int ychange, int zchange, unsigned int cliptype);
	void (*lotsofmoney)(spritetype *s, short n);
	void (*lotsofmail)(spritetype *s, short n);
	void (*lotsofpaper)(spritetype *s, short n);
	void (*guts)(spritetype* s, short gtype, short n, short p);
	void (*gutsdir)(spritetype* s, short gtype, short n, short p);
	int  (*ifhitsectors)(int sectnum);
	int  (*ifhitbyweapon)(int sectnum);
	void (*fall)(int g_i, int g_p);
	bool (*spawnweapondebris)(int picnum, int dnum);
	void (*respawnhitag)(spritetype* g_sp);
	void (*checktimetosleep)(int g_i);
	void (*move)(int g_i, int g_p, int g_x);
	int (*spawn)(int j, int pn);
	void (*check_fta_sounds)(int i);

	// player
	void (*incur_damage)(struct player_struct* p);
	void (*shoot)(int, int);
	void (*selectweapon)(int snum, int j);
	int (*doincrements)(struct player_struct* p);
	void (*checkweapons)(struct player_struct* p);
	void (*processinput)(int snum);
	void (*displayweapon)(int snum);
	void (*displaymasks)(int snum);

	void (*animatesprites)(int x, int y, int a, int smoothratio);


};

extern Dispatcher fi;

END_DUKE_NS

#endif
