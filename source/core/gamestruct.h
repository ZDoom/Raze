#pragma once

bool System_WantGuiCapture();	// During playing this tells us whether the game must be paused due to active GUI elememts.

#include <stdint.h>
#include "vectors.h"
#include "engineerrors.h"
#include "stats.h"
#include "serializer.h"
#include "inputstate.h"
#include "maptypes.h"

class FSerializer;
struct FRenderViewpoint;
struct sectortype;
struct tspritetype;
struct MapRecord;

struct GameStats
{
	int kill, tkill;
	int secret, tsecret;
	int timesecnd;
	int frags;
};

struct FNewGameStartup
{
	MapRecord* Map;
	int Episode;
	int Level;
	int Skill;
};

struct FSavegameInfo
{
	const char *savesig;
	int minsavever;
	int currentsavever;
};

enum EMenuSounds : int;
struct MapRecord;
struct TilesetBuildInfo;

struct GeoEffect
{
	sectortype** geosectorwarp;
	sectortype** geosectorwarp2;
	sectortype** geosector;
	double* geox;
	double* geoy;
	double* geox2;
	double* geoy2;
	int geocnt;

};

struct GameInterface
{
	virtual const char* Name() { return "$"; }
	virtual ~GameInterface() {}
	virtual bool GenerateSavePic() { return false; }
	virtual void app_init() = 0;
	virtual void FinalizeSetup() {}
	virtual void LoadTextureInfo(TilesetBuildInfo& info) {}
	virtual void SetupSpecialTextures(TilesetBuildInfo&) = 0;
	virtual void loadPalette() = 0;
	virtual void FreeLevelData();
	virtual void PlayHudSound() {}
	virtual void MenuOpened() {}
	virtual void MenuClosed() {}
	virtual void MenuSound(EMenuSounds snd) {}
	virtual bool CanSave() { return true; }
	virtual bool StartGame(FNewGameStartup& gs) { return true; }
	virtual FSavegameInfo GetSaveSig() { return { "", 0, 0}; }
	virtual double SmallFontScale() { return 1; }
	virtual void SerializeGameState(FSerializer& arc) = 0;
	virtual void DrawPlayerSprite(const DVector2& origin, bool onteam) {}
	virtual void SetAmbience(bool on) {}
	virtual void ExitFromMenu() { throw CExitEvent(0); }
	virtual void UpdateSounds() = 0;
	virtual void ErrorCleanup() {}
	virtual void Startup() = 0;
	virtual void DrawBackground() = 0;
	virtual void Render() = 0;
	virtual void Ticker() = 0;
	virtual int GetPlayerChecksum(int pnum) { return 0x12345678 + pnum; }
	virtual const char *CheckCheatMode() { return nullptr; }
	virtual const char* GenericCheat(int player, int cheat) = 0;
	virtual void NextLevel(MapRecord* map, int skill) = 0;
	virtual void NewGame(MapRecord* map, int skill, bool special = false) = 0;
	virtual void LevelCompleted(MapRecord* map, int skill) = 0;
	virtual bool DrawAutomapPlayer(const DVector2& mxy, const DVector2& cpos, const DAngle cang, const DVector2& xydim, const double czoom, double const interpfrac) { return false; }
	virtual DAngle playerPitchMin() { return DAngle::fromDeg(57.375); }
	virtual DAngle playerPitchMax() { return DAngle::fromDeg(-57.375); }
	virtual void ToggleThirdPerson() = 0;
	virtual void SwitchCoopView() { Printf("Unsupported command\n"); }
	virtual void ToggleShowWeapon() { Printf("Unsupported command\n"); }
	virtual void processSprites(tspriteArray& tsprites, const DVector3& view, DAngle viewang, double interpfrac) = 0;
	virtual void UpdateCameras(double smoothratio) {}
	virtual void EnterPortal(DCoreActor* viewer, int type) {}
	virtual void LeavePortal(DCoreActor* viewer, int type) {}
	virtual bool GetGeoEffect(GeoEffect* eff, sectortype* viewsector) { return false; }
	virtual int Voxelize(int sprnum) { return -1; }
	virtual void AddExcludedEpisode(const FString& episode) {}
	virtual int GetCurrentSkill() { return -1; }
	virtual bool IsQAVInterpTypeValid(const FString& type) { return false; }
	virtual void AddQAVInterpProps(const int res_id, const FString& interptype, const bool loopable, const TMap<int, TArray<int>>&& ignoredata) { }
	virtual void RemoveQAVInterpProps(const int res_id) { }
	virtual bool WantEscape() { return false; }
	virtual void StartSoundEngine() = 0;
	virtual void doPlayerMovement();
	virtual unsigned getCrouchState() = 0;
};

extern GameInterface* gi;
