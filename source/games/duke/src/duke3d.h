#pragma once

#include "build.h"

#include "compat.h"

#include "pragmas.h"

#include "polymost.h"
#include "gamecvars.h"
#include "razemenu.h"
#include "gamecontrol.h"
#include "gamevar.h"
#include "global.h"
#include "funct.h"
#include "names.h"
#include "quotemgr.h"
#include "rts.h"
#include "sounds.h"
#include "soundefs.h"
#include "gamestruct.h"
#include "v_draw.h"

BEGIN_DUKE_NS

extern FFont* IndexFont;
extern FFont* DigiFont;

struct GameInterface : public ::GameInterface
{
	const char* Name() override { return "Duke"; }
	void app_init() override;
	void clearlocalinputstate() override;
	bool GenerateSavePic() override;
	void PlayHudSound() override;
	GameStats getStats() override;
	void MenuOpened() override;
	void MenuSound(EMenuSounds snd) override;
	bool CanSave() override;
	bool StartGame(FNewGameStartup& gs) override;
	FSavegameInfo GetSaveSig() override;
	double SmallFontScale() override { return isRR() ? 0.5 : 1.; }
	void SerializeGameState(FSerializer& arc) override;
	void QuitToTitle() override;
	FString GetCoordString() override;
	void ExitFromMenu() override;
	ReservedSpace GetReservedScreenSpace(int viewsize) override;
	void DrawPlayerSprite(const DVector2& origin, bool onteam) override;
	void GetInput(InputPacket* packet, ControlInfo* const hidInput) override;
	void UpdateSounds() override;
	void Startup() override;
	void DrawBackground() override;
	void Render() override;
	void Ticker() override;
	const char* GenericCheat(int player, int cheat) override;
	const char* CheckCheatMode() override;
	void NextLevel(MapRecord* map, int skill) override;
	void NewGame(MapRecord* map, int skill) override;
	void LevelCompleted(MapRecord* map, int skill) override;
	bool DrawAutomapPlayer(int x, int y, int z, int a) override;
	int playerKeyMove() override { return 40; }

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
	void (*checkhitwall)(DDukeActor* spr, int dawallnum, int x, int y, int z, int atwith);
	void (*checkplayerhurt)(struct player_struct* p, int j);
	bool (*checkhitceiling)(int sn);
	void (*checkhitsprite)(DDukeActor* i, DDukeActor* sn);
	void (*checksectors)(int low);

	bool (*ceilingspace)(int sectnum);
	bool (*floorspace)(int sectnum);
	void (*addweapon)(struct player_struct *p, int weapon);
	void (*hitradius)(DDukeActor* i, int  r, int  hp1, int  hp2, int  hp3, int  hp4);
	int  (*movesprite)(int spritenum, int xchange, int ychange, int zchange, unsigned int cliptype);
	void (*lotsofmoney)(DDukeActor *s, short n);
	void (*lotsofmail)(DDukeActor *s, short n);
	void (*lotsofpaper)(DDukeActor *s, short n);
	void (*guts)(DDukeActor* s, short gtype, short n, short p);
	DDukeActor* (*ifhitsectors)(int sectnum);
	int  (*ifhitbyweapon)(DDukeActor* sectnum);
	void (*fall)(DDukeActor* actor, int g_p);
	bool (*spawnweapondebris)(int picnum, int dnum);
	void (*respawnhitag)(DDukeActor* g_sp);
	void (*checktimetosleep)(DDukeActor* actor);
	void (*move)(DDukeActor* i, int g_p, int g_x);
	int (*spawn)(int j, int pn);

	// player
	void (*incur_damage)(struct player_struct* p);
	void (*shoot)(int, int);
	void (*selectweapon)(int snum, int j);
	int (*doincrements)(struct player_struct* p);
	void (*checkweapons)(struct player_struct* p);
	void (*processinput)(int snum);
	void (*displayweapon)(int snum, double smoothratio);
	void (*displaymasks)(int snum, double smoothratio);

	void (*animatesprites)(int x, int y, int a, int smoothratio);


};

extern Dispatcher fi;

END_DUKE_NS

