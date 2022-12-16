#pragma once

#include "build.h"

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
#include "gamefuncs.h"

BEGIN_DUKE_NS

extern player_struct ps[MAXPLAYERS];

struct GameInterface : public ::GameInterface
{
	const char* Name() override { return "Duke"; }
	void app_init() override;
	void loadPalette() override;
	void SetupSpecialTextures(TilesetBuildInfo& info) override;
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
	void ExitFromMenu() override;
	void DrawPlayerSprite(const DVector2& origin, bool onteam) override;
	void reapplyInputBits(InputPacket* const input) override { input->actions |= ps[myconnectindex].sync.actions & SB_CENTERVIEW; }
	void doPlayerMovement(const float scaleAdjust) override;
	void UpdateSounds() override;
	void Startup() override;
	void DrawBackground() override;
	void Render() override;
	void Ticker() override;
	const char* GenericCheat(int player, int cheat) override;
	const char* CheckCheatMode() override;
	void NextLevel(MapRecord* map, int skill) override;
	void NewGame(MapRecord* map, int skill, bool) override;
	void LevelCompleted(MapRecord* map, int skill) override;
	bool DrawAutomapPlayer(const DVector2& mxy, const DVector2& cpos, const DAngle cang, const DVector2& xydim, const double czoom, double const interpfrac) override;
	DCoreActor* getConsoleActor() override { return ps[myconnectindex].GetActor(); }
	void ToggleThirdPerson() override;
	void SwitchCoopView() override;
	void ToggleShowWeapon() override;
	void processSprites(tspriteArray& tsprites, const DVector3& view, DAngle viewang, double interpfrac) override;
	void UpdateCameras(double smoothratio) override;
	void EnterPortal(DCoreActor* viewer, int type) override;
	void LeavePortal(DCoreActor* viewer, int type) override;
	bool GetGeoEffect(GeoEffect* eff, sectortype* viewsector) override;
	void AddExcludedEpisode(const FString& episode) override;
	int GetCurrentSkill() override;
	bool WantEscape() override;
	void StartSoundEngine() override;

};

struct Dispatcher
{
	// sectors_?.cpp
	void (*think)();
	void (*movetransports)();
	void (*initactorflags)();
	bool (*checkaccessswitch)(int snum, int switchpal, DDukeActor* act, walltype* w);
	void (*activatebysector)(sectortype* sect, DDukeActor* j);
	void (*checkhitsprite)(DDukeActor* i, DDukeActor* sn);
	void (*checkhitdefault)(DDukeActor* i, DDukeActor* sn);
	void (*checksectors)(int low);

	void (*addweapon)(player_struct *p, int weapon, bool wswitch);
	void (*hitradius)(DDukeActor* i, int  r, int  hp1, int  hp2, int  hp3, int  hp4);
	void (*lotsofmoney)(DDukeActor *s, int n);
	void (*lotsofmail)(DDukeActor *s, int n);
	void (*lotsofpaper)(DDukeActor *s, int n);
	int  (*ifhitbyweapon)(DDukeActor* sectnum);
	void (*fall)(DDukeActor* actor, int g_p);
	bool (*spawnweapondebris)(int picnum);
	void (*move)(DDukeActor* i, int g_p, int g_x);

	// player
	void (*incur_damage)(player_struct* p);
	void (*shoot)(DDukeActor*, int, PClass* cls);
	void (*selectweapon)(int snum, int j);
	int (*doincrements)(player_struct* p);
	void (*checkweapons)(player_struct* p);
	void (*processinput)(int snum);
	void (*displayweapon)(int snum, double interpfrac);
	void (*displaymasks)(int snum, int p, double interpfrac);

	void (*animatesprites)(tspriteArray& tsprites, const DVector2& viewVec, DAngle viewang, double interpfrac);


};

extern Dispatcher fi;

void CallInitialize(DDukeActor* actor);
void CallTick(DDukeActor* actor);
bool CallOperate(DDukeActor* actor, int plnum);
void CallAction(DDukeActor* actor);
void CallOnHit(DDukeActor* actor, DDukeActor* hitter);
void CallOnHurt(DDukeActor* actor, player_struct* hitter);
void CallOnTouch(DDukeActor* actor, player_struct* hitter);
bool CallOnUse(DDukeActor* actor, player_struct* user);
void CallOnMotoSmash(DDukeActor* actor, player_struct* hitter);
void CallOnRespawn(DDukeActor* actor, int low);
bool CallAnimate(DDukeActor* actor, tspritetype* hitter);
bool CallShootThis(DDukeActor* clsdef, DDukeActor* actor, int pn, const DVector3& spos, DAngle sang);
void CallStaticSetup(DDukeActor* actor);
void CallPlayFTASound(DDukeActor* actor);
void CallStandingOn(DDukeActor* actor, player_struct* p);
void CallRunState(DDukeActor* actor);
int CallTriggerSwitch(DDukeActor* actor, player_struct* p);
PClassActor* CallGetRadiusDamageType(DDukeActor* actor, int targhealth);



extern FTextureID mirrortex, foftex;

END_DUKE_NS

