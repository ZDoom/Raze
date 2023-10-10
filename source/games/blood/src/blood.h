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
#pragma once

#include "build.h"
#include "vm.h"
#include "gamestruct.h"
#include "mapinfo.h"
#include "d_net.h"
#include "serialize_obj.h"

#include "common_game.h"
#include "fx.h"
#include "gameutil.h"
#include "db.h"

#include "actor.h"
#include "ai.h"
#include "aiunicult.h"
#include "callback.h"
#include "db.h"
#include "endgame.h"
#include "eventq.h"
#include "gib.h"
#include "globals.h"
#include "levels.h"
#include "misc.h"
#include "player.h"
#include "seq.h"
#include "sound.h"
#include "triggers.h"
#include "view.h"
#include "nnexts.h"
#include "player.h"
#include "misc.h"
#include "sectorfx.h"
#include "bloodactor.h"


BEGIN_BLD_NS

// ai callbacks
DEF_ANIMATOR(aiMoveDodge)
DEF_ANIMATOR(aiMoveForward)
DEF_ANIMATOR(aiMoveTurn)
DEF_ANIMATOR(aiPodChase)
DEF_ANIMATOR(aiPodMove)
DEF_ANIMATOR(aiPodSearch)
DEF_ANIMATOR(aiThinkTarget)
DEF_ANIMATOR(batMoveDodgeDown)
DEF_ANIMATOR(batMoveDodgeUp)
DEF_ANIMATOR(batMoveFly)
DEF_ANIMATOR(batMoveForward)
DEF_ANIMATOR(batMoveSwoop)
DEF_ANIMATOR(batMoveToCeil)
DEF_ANIMATOR(batThinkChase)
DEF_ANIMATOR(batThinkGoto)
DEF_ANIMATOR(batThinkPonder)
DEF_ANIMATOR(batThinkSearch)
DEF_ANIMATOR(batThinkTarget)
DEF_ANIMATOR(beastMoveForward)
DEF_ANIMATOR(beastThinkChase)
DEF_ANIMATOR(beastThinkGoto)
DEF_ANIMATOR(beastThinkSearch)
DEF_ANIMATOR(beastThinkSwimChase)
DEF_ANIMATOR(beastThinkSwimGoto)
DEF_ANIMATOR(burnThinkChase)
DEF_ANIMATOR(burnThinkGoto)
DEF_ANIMATOR(burnThinkSearch)
DEF_ANIMATOR(calebThinkChase)
DEF_ANIMATOR(calebThinkGoto)
DEF_ANIMATOR(calebThinkSearch)
DEF_ANIMATOR(calebThinkSwimChase)
DEF_ANIMATOR(calebThinkSwimGoto)
DEF_ANIMATOR(cerberusThinkChase)
DEF_ANIMATOR(cerberusThinkGoto)
DEF_ANIMATOR(cerberusThinkSearch)
DEF_ANIMATOR(cerberusThinkTarget)
DEF_ANIMATOR(cultThinkChase)
DEF_ANIMATOR(cultThinkGoto)
DEF_ANIMATOR(cultThinkSearch)
DEF_ANIMATOR(eelMoveAscend)
DEF_ANIMATOR(eelMoveDodgeDown)
DEF_ANIMATOR(eelMoveDodgeUp)
DEF_ANIMATOR(eelMoveForward)
DEF_ANIMATOR(eelMoveSwoop)
DEF_ANIMATOR(eelMoveToCeil)
DEF_ANIMATOR(eelThinkChase)
DEF_ANIMATOR(eelThinkGoto)
DEF_ANIMATOR(eelThinkPonder)
DEF_ANIMATOR(eelThinkSearch)
DEF_ANIMATOR(eelThinkTarget)
DEF_ANIMATOR(entryAIdle)
DEF_ANIMATOR(entryEStand)
DEF_ANIMATOR(entryEZombie)
DEF_ANIMATOR(entryFStatue)
DEF_ANIMATOR(entrySStatue)
DEF_ANIMATOR(gargMoveDodgeDown)
DEF_ANIMATOR(gargMoveDodgeUp)
DEF_ANIMATOR(gargMoveFly)
DEF_ANIMATOR(gargMoveForward)
DEF_ANIMATOR(gargMoveSlow)
DEF_ANIMATOR(gargMoveSwoop)
DEF_ANIMATOR(gargThinkChase)
DEF_ANIMATOR(gargThinkGoto)
DEF_ANIMATOR(gargThinkSearch)
DEF_ANIMATOR(gargThinkTarget)
DEF_ANIMATOR(ghostMoveDodgeDown)
DEF_ANIMATOR(ghostMoveDodgeUp)
DEF_ANIMATOR(ghostMoveFly)
DEF_ANIMATOR(ghostMoveForward)
DEF_ANIMATOR(ghostMoveSlow)
DEF_ANIMATOR(ghostMoveSwoop)
DEF_ANIMATOR(ghostThinkChase)
DEF_ANIMATOR(ghostThinkGoto)
DEF_ANIMATOR(ghostThinkSearch)
DEF_ANIMATOR(ghostThinkTarget)
DEF_ANIMATOR(gillThinkChase)
DEF_ANIMATOR(gillThinkGoto)
DEF_ANIMATOR(gillThinkSearch)
DEF_ANIMATOR(gillThinkSwimChase)
DEF_ANIMATOR(gillThinkSwimGoto)
DEF_ANIMATOR(handThinkChase)
DEF_ANIMATOR(handThinkGoto)
DEF_ANIMATOR(handThinkSearch)
DEF_ANIMATOR(houndThinkChase)
DEF_ANIMATOR(houndThinkGoto)
DEF_ANIMATOR(houndThinkSearch)
DEF_ANIMATOR(innocThinkChase)
DEF_ANIMATOR(innocThinkGoto)
DEF_ANIMATOR(innocThinkSearch)
DEF_ANIMATOR(MorphToBeast)
DEF_ANIMATOR(myThinkSearch)
DEF_ANIMATOR(myThinkTarget)
DEF_ANIMATOR(playStatueBreakSnd)
DEF_ANIMATOR(ratThinkChase)
DEF_ANIMATOR(ratThinkGoto)
DEF_ANIMATOR(ratThinkSearch)
DEF_ANIMATOR(spidThinkChase)
DEF_ANIMATOR(spidThinkGoto)
DEF_ANIMATOR(spidThinkSearch)
DEF_ANIMATOR(beastMoveSwim)
DEF_ANIMATOR(beastMoveSwimAlt)
DEF_ANIMATOR(beastMoveIn)
DEF_ANIMATOR(calebMoveSwimChase)
DEF_ANIMATOR(calebSwimUnused)
DEF_ANIMATOR(calebSwimMoveIn)
DEF_ANIMATOR(gillMoveSwimChase)
DEF_ANIMATOR(gillMoveSwimUnused)
DEF_ANIMATOR(gillSwimMoveIn)
DEF_ANIMATOR(tchernobogThinkSearch)
DEF_ANIMATOR(tchernobogThinkTarget)
DEF_ANIMATOR(tchernobogThinkGoto)
DEF_ANIMATOR(tchernobogThinkChase)
DEF_ANIMATOR(zombaThinkChase)
DEF_ANIMATOR(zombaThinkGoto)
DEF_ANIMATOR(zombaThinkPonder)
DEF_ANIMATOR(zombaThinkSearch)
DEF_ANIMATOR(zombfThinkChase)
DEF_ANIMATOR(zombfThinkGoto)
DEF_ANIMATOR(zombfThinkSearch)
// seq callbacks
DEF_ANIMATOR(FireballSeqCallback)
DEF_ANIMATOR(Fx33Callback)
DEF_ANIMATOR(NapalmSeqCallback)
DEF_ANIMATOR(Fx32Callback)
DEF_ANIMATOR(TreeToGibCallback)
DEF_ANIMATOR(DudeToGibCallback1)
DEF_ANIMATOR(DudeToGibCallback2)
DEF_ANIMATOR(batBiteSeqCallback)
DEF_ANIMATOR(SlashSeqCallback)
DEF_ANIMATOR(StompSeqCallback)
DEF_ANIMATOR(eelBiteSeqCallback)
DEF_ANIMATOR(BurnSeqCallback)
DEF_ANIMATOR(SeqAttackCallback)
DEF_ANIMATOR(cerberusBiteSeqCallback)
DEF_ANIMATOR(cerberusBurnSeqCallback)
DEF_ANIMATOR(cerberusBurnSeqCallback2)
DEF_ANIMATOR(TommySeqCallback)
DEF_ANIMATOR(TeslaSeqCallback)
DEF_ANIMATOR(ShotSeqCallback)
DEF_ANIMATOR(cultThrowSeqCallback)
DEF_ANIMATOR(cultThrowSeqCallback2)
DEF_ANIMATOR(cultThrowSeqCallback3)
DEF_ANIMATOR(SlashFSeqCallback)
DEF_ANIMATOR(ThrowFSeqCallback)
DEF_ANIMATOR(BlastSSeqCallback)
DEF_ANIMATOR(ThrowSSeqCallback)
DEF_ANIMATOR(ghostSlashSeqCallback)
DEF_ANIMATOR(ghostThrowSeqCallback)
DEF_ANIMATOR(ghostBlastSeqCallback)
DEF_ANIMATOR(GillBiteSeqCallback)
DEF_ANIMATOR(HandJumpSeqCallback)
DEF_ANIMATOR(houndBiteSeqCallback)
DEF_ANIMATOR(houndBurnSeqCallback)
DEF_ANIMATOR(podPlaySound1)
DEF_ANIMATOR(podPlaySound2)
DEF_ANIMATOR(podAttack)
DEF_ANIMATOR(podExplode)
DEF_ANIMATOR(ratBiteSeqCallback)
DEF_ANIMATOR(SpidBiteSeqCallback)
DEF_ANIMATOR(SpidJumpSeqCallback)
DEF_ANIMATOR(SpidBirthSeqCallback)
DEF_ANIMATOR(tchernobogFire)
DEF_ANIMATOR(tchernobogBurnSeqCallback)
DEF_ANIMATOR(tchernobogBurnSeqCallback2)
DEF_ANIMATOR(HackSeqCallback)
DEF_ANIMATOR(StandSeqCallback)
DEF_ANIMATOR(zombfHackSeqCallback)
DEF_ANIMATOR(PukeSeqCallback)
DEF_ANIMATOR(ThrowSeqCallback)
DEF_ANIMATOR(PlayerSurvive)
DEF_ANIMATOR(PlayerKneelsOver)
DEF_ANIMATOR(FireballTrapSeqCallback)
DEF_ANIMATOR(MGunFireSeqCallback)
DEF_ANIMATOR(MGunOpenSeqCallback)


// event callbacks
DEF_ANIMATOR(fxFlameLick) // 0
DEF_ANIMATOR(RemoveActor) // 1
DEF_ANIMATOR(FlareBurst) // 2
DEF_ANIMATOR(fxFlareSpark) // 3
DEF_ANIMATOR(fxFlareSparkLite) // 4
DEF_ANIMATOR(fxZombieBloodSpurt) // 5
DEF_ANIMATOR(fxBloodSpurt) // 6
DEF_ANIMATOR(fxArcSpark) // 7
DEF_ANIMATOR(fxDynPuff) // 8
DEF_ANIMATOR(Respawn) // 9
DEF_ANIMATOR(PlayerBubble) // 10
DEF_ANIMATOR(EnemyBubble) // 11
DEF_ANIMATOR(FinishHim) // 13
DEF_ANIMATOR(fxBloodBits) // 14
DEF_ANIMATOR(fxTeslaAlt) // 15
DEF_ANIMATOR(fxBouncingSleeve) // 16
DEF_ANIMATOR(returnFlagToBase) // 17
DEF_ANIMATOR(fxPodBloodSpray) // 18
DEF_ANIMATOR(fxPodBloodSplat) // 19
DEF_ANIMATOR(LeechStateTimer) // 20
DEF_ANIMATOR(DropVoodooCb) // unused
DEF_ANIMATOR(callbackMakeMissileBlocking) // 23
DEF_ANIMATOR(callbackMissileBurst)

#ifdef NOONE_EXTENSIONS
DEF_ANIMATOR(forcePunch)
DEF_ANIMATOR(aiGenDudeMoveForward)
DEF_ANIMATOR(unicultThinkChase)
DEF_ANIMATOR(unicultThinkGoto)
DEF_ANIMATOR(unicultThinkSearch)
DEF_ANIMATOR(genDudeAttack1)
DEF_ANIMATOR(punchCallback)
DEF_ANIMATOR(ThrowCallback1)
DEF_ANIMATOR(ThrowCallback2)
DEF_ANIMATOR(callbackGenDudeUpdate)
#endif


enum EFeatureFlags
{
	kFeatureCustomAmmoCount = 1,
	kFeatureEnemyAttacks = 2,
	kFeatureCustomClipdist = 4,
	kFeatureCustomTrapExploder = 8,
	kFeatureCustomEnemyHealth = 16,
	kFeaturePlayerSize = 32,
};

constexpr int BMAX_PATH = 260;

struct INIDESCRIPTION {
	const char* pzName;
	const char* pzFilename;
	const char** pzArts;
	int nArts;
};

struct INICHAIN {
	INICHAIN* pNext;
	char zName[BMAX_PATH];
	INIDESCRIPTION* pDescription;
};

extern INICHAIN* pINIChain;

extern int gNetPlayers;
extern int blood_globalflags;

void QuitGame(void);
void PreloadCache(void);
void ProcessFrame(void);
void ScanINIFiles(void);
void EndLevel(bool);

extern int mirrorcnt, mirrorsector, mirrorwall[4];

inline bool DemoRecordStatus(void)
{
	return false;
}

inline bool VanillaMode()
{
	return false;
}
void sndPlaySpecialMusicOrNothing(int nMusic);

struct GameInterface : public ::GameInterface
{
	const char* Name() override { return "Blood"; }
	void app_init() override;
	void SerializeGameState(FSerializer& arc) override;
	void loadPalette() override;
	bool GenerateSavePic() override;
	void FreeLevelData() override;
	FSavegameInfo GetSaveSig() override;
	void MenuOpened() override;
	void MenuClosed() override;
	bool CanSave() override;
	void UpdateSounds() override;
	void Ticker() override;
	void DrawBackground() override;
	void Startup() override;
	void Render() override;
	const char* GenericCheat(int player, int cheat) override;
	void NewGame(MapRecord* sng, int skill, bool) override;
	void NextLevel(MapRecord* map, int skill) override;
	void LevelCompleted(MapRecord* map, int skill) override;
	bool DrawAutomapPlayer(const DVector2& mxy, const DVector2& cpos, const DAngle cang, const DVector2& xydim, const double czoom, double const interpfrac) override;
	DAngle playerPitchMin() override { return DAngle::fromDeg(54.575); }
	DAngle playerPitchMax() override { return DAngle::fromDeg(-43.15); }
	void ToggleThirdPerson() override;
	void SwitchCoopView() override;
	void ToggleShowWeapon() override;
	void processSprites(tspriteArray& tsprites, const DVector3& view, DAngle viewang, double interpfrac) override;
	void EnterPortal(DCoreActor* viewer, int type) override;
	void LeavePortal(DCoreActor* viewer, int type) override;
	void LoadTextureInfo(TilesetBuildInfo& info) override;
	void SetupSpecialTextures(TilesetBuildInfo&) override;
	int GetCurrentSkill() override;
	bool IsQAVInterpTypeValid(const FString& type) override;
	void AddQAVInterpProps(const int res_id, const FString& interptype, const bool loopable, const TMap<int, TArray<int>>&& ignoredata) override;
	void RemoveQAVInterpProps(const int res_id) override;
	void StartSoundEngine() override;
	unsigned getCrouchState() override;
	void FinalizeSetup() override;
};

END_BLD_NS
