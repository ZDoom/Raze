#pragma once

#include "game.h"

// This consolidates the contents of several smaller includes
BEGIN_SW_NS

enum
{
	CACHE_SOUND_PRECACHE = 0,
	CACHE_SOUND_PLAY =1
};

void DoTheCache(void);

void InitCheats();

void MapColors(short num,COLOR_MAP cm,short create);
int32_t CONFIG_ReadSetup(void);

bool WarpSectorInfo(sectortype* sect, DSWActor** sp_warp);
DSWActor* Warp(DVector3& pos, sectortype** sect);
DSWActor* WarpPlane(DVector3& pos, sectortype** sect, double testz = DBL_MAX);



void ProcessVisOn(void);
void VisViewChange(DSWPlayer* pp, int* vis);
void SpawnVis(DSWActor* Parent, sectortype* sect, const DVector3& pos, int amt);

enum TriggerType { TRIGGER_TYPE_REMOTE_SO };

int ActorFollowTrack(DSWActor*, short locktics);
void ActorLeaveTrack(DSWActor*);
void RefreshPoints(SECTOR_OBJECT* sop, const DVector2& pos, bool dynamic);
void TrackSetup(void);
void PlaceSectorObject(SECTOR_OBJECT* sop, const DVector2& newpos);
void PlaceSectorObjectsOnTracks(void);
void PlaceActorsOnTracks(void);
void SetupSectorObject(sectortype* sect, short tag);
void PostSetupSectorObject(void);
void VehicleSetSmoke(SECTOR_OBJECT* sop, VMFunction* animator);
void CollapseSectorObject(SECTOR_OBJECT* sop, const DVector2& pos);
void KillSectorObjectSprites(SECTOR_OBJECT* sop);
void MoveSectorObjects(SECTOR_OBJECT* sop, short locktics);

int DoActorBeginJump(DSWActor* actor);
int DoActorBeginFall(DSWActor* actor);
int DoActorFall(DSWActor* actor);

#define TEXT_INFO_TIME (3)
#define TEXT_INFO_Y (40)
#define TEXT_INFO_YOFF (10)
inline constexpr int TEXT_INFO_LINE(int line) { return (TEXT_INFO_Y + ((line)*TEXT_INFO_YOFF)); }

void PutStringInfo(DSWPlayer* pp, const char* string);


void DoSlidorMatch(DSWPlayer* pp, short match, bool);
bool TestSlidorMatchActive(short match);
void InterpSectorSprites(sectortype* sect, bool state);

using INTERP_FUNC = void(*)(walltype*, int);

void SetSlidorActive(DSWActor*);
void DoSlidorInterp(DSWActor*, INTERP_FUNC);

int DoBeginJump(DSWActor* actor);
int DoJump(DSWActor* actor);
int DoBeginFall(DSWActor* actor);
int DoFall(DSWActor* actor);
void KeepActorOnFloor(DSWActor* actor);
int DoActorSlide(DSWActor* actor);
int DoActorSectorDamage(DSWActor* actor);
int DoScaleSprite(DSWActor* actor);
int DoActorStopFall(DSWActor* actor);

void InitPlayerSprite(DSWPlayer* pp, const DVector3& spawnpos, const DAngle startang);
void InitAllPlayerSprites(const DVector3& spawnpos, const DAngle startang);
void PlayerPanelSetup(void);
void PlayerDeathReset(DSWPlayer* pp);
void SpawnPlayerUnderSprite(DSWPlayer* pp);

void DoQuakeMatch(short match);
void ProcessQuakeOn(void);
void ProcessQuakeSpot(void);
void QuakeViewChange(DSWPlayer* pp, DVector3& tpos, DAngle& tang);
void DoQuake(DSWPlayer* pp);
bool SetQuake(DSWPlayer* pp, short tics, short amt);
int SetExpQuake(DSWActor*);
int SetGunQuake(DSWActor*);
int SetPlayerQuake(DSWPlayer* mpp);
int SetNuclearQuake(DSWActor*);
int SetSumoQuake(DSWActor*);
int SetSumoFartQuake(DSWActor*);

END_SW_NS
