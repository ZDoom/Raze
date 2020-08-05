#pragma once

#include "game.h"

// This consolidates the contents of several smaller includes
BEGIN_SW_NS

enum
{
	CACHE_SOUND_PRECACHE = 0,
	CACHE_SOUND_PLAY =1
};

void SetupPreCache(void);
void PreCacheRange(short start_pic, short end_pic);
void DoTheCache(void);
void precache(void);


void KeysCheat(PLAYERstruct * pp, const char *cheat_string);
void ResCheatOn(PLAYERstruct *, const char *);
void VoxCheat(PLAYERstruct *, const char *);
void RestartCheat(PLAYERstruct *, const char *);
void RoomCheat(PLAYERstruct *, const char *);
void SecretCheat(PLAYERstruct * pp, const char *);
void NextCheat(PLAYERstruct * pp, const char *);
void PrevCheat(PLAYERstruct * pp, const char *);
void MapCheat(PLAYERstruct * pp, const char *);
void LocCheat(PLAYERstruct * pp, const char *);
void GunsCheat(PLAYERstruct * pp, const char *cheat_string);
void AmmoCheat(PLAYERstruct * pp, const char *);
void WeaponCheat(PLAYERstruct * pp, const char *);
void GodCheat(PLAYERstruct * pp, const char *);
void ClipCheat(PLAYERstruct * pp, const char *);
void WarpCheat(PLAYERstruct * pp, const char *cheat_string);
void ItemCheat(PLAYERstruct * pp, const char *cheat_string);
void InventoryCheat(PLAYERstruct * pp, const char *cheat_string);
void ArmorCheat(PLAYERstruct * pp, const char *cheat_string);
void HealCheat(PLAYERstruct * pp, const char *cheat_string);
void SortKeyCheat(PLAYERstruct * pp, const char *sKey);
void KeysCheat(PLAYERstruct * pp, const char *cheat_string);
void EveryCheatToggle(PLAYERstruct * pp, const char *cheat_string);
void GeorgeFunc(PLAYERstruct * pp, char *);
void BlackburnFunc(PLAYERstruct * pp, char *);
int cheatcmp(const char *str1, const char *str2, int len);
void CheatInput(void);

void MapColors(short num,COLOR_MAP cm,short create);
void InitPalette(void); 
int32_t CONFIG_ReadSetup(void);

SWBOOL WarpPlaneSectorInfo(short sectnum, SPRITEp* sp_ceiling, SPRITEp* sp_floor);
SPRITEp WarpPlane(int32_t* x, int32_t* y, int32_t* z, int16_t* sectnum);
SPRITEp WarpToArea(SPRITEp sp_from, int32_t* x, int32_t* y, int32_t* z, int16_t* sectnum);
SWBOOL WarpSectorInfo(short sectnum, SPRITEp* sp_warp);
SPRITEp Warp(int32_t* x, int32_t* y, int32_t* z, int16_t* sectnum);

void ProcessVisOn(void);
void VisViewChange(PLAYERp pp, int* vis);
int SpawnVis(short Parent, short sectnum, int x, int y, int z, int amt);

enum TriggerType { TRIGGER_TYPE_REMOTE_SO };

int ActorFollowTrack(short SpriteNum, short locktics);
void ActorLeaveTrack(short SpriteNum);
void RefreshPoints(SECTOR_OBJECTp sop, int nx, int ny, SWBOOL dynamic);
void TrackSetup(void);
void PlaceSectorObject(SECTOR_OBJECTp sop, int newx, int newy);
void PlaceSectorObjectsOnTracks(void);
void PlaceActorsOnTracks(void);
void SetupSectorObject(short sectnum, short tag);
void PostSetupSectorObject(void);
void VehicleSetSmoke(SECTOR_OBJECTp sop, ANIMATORp animator);
void CollapseSectorObject(SECTOR_OBJECTp sop, int nx, int ny);
void KillSectorObjectSprites(SECTOR_OBJECTp sop);
void MoveSectorObjects(SECTOR_OBJECTp sop, short locktics);

#define TEXT_INFO_TIME (3)
#define TEXT_INFO_Y (40)
#define TEXT_INFO_YOFF (10)
inline int TEXT_INFO_LINE(int line) { return (TEXT_INFO_Y + ((line)*TEXT_INFO_YOFF)); }

void DisplayFragNames(PLAYERp pp);
void DisplayMiniBarSmString(PLAYERp pp, short xs, short ys, short pal, const char* buffer);
void DisplaySmString(PLAYERp pp, short xs, short ys, short pal, const char* buffer);
void DisplayMiniBarNumber(short xs, short ys, int number);
void DisplaySummaryString(PLAYERp pp, short xs, short ys, short color, short shade, const char* buffer);
void DisplayPanelNumber(PLAYERp pp, short xs, short ys, int number);
void PutStringInfo(PLAYERp pp, const char* string);
void PutStringInfoLine(PLAYERp pp, const char* string);
void PutStringInfoLine2(PLAYERp pp, const char* string);
void pClearTextLine(PLAYERp pp, int y);
void pMenuClearTextLine(PLAYERp pp);

void StringTimer(PANEL_SPRITEp psp);

short DoSlidorMatch(PLAYERp pp, short match, SWBOOL);
SWBOOL TestSlidorMatchActive(short match);
void InterpSectorSprites(short sectnum, SWBOOL state);

typedef void INTERP_FUNC(int*);
typedef INTERP_FUNC* INTERP_FUNCp;

void SetSlidorActive(short SpriteNum);
void DoSlidorInterp(short, INTERP_FUNCp);

int DoBeginJump(short SpriteNum);
int DoJump(short SpriteNum);
int DoBeginFall(short SpriteNum);
int DoFall(short SpriteNum);
void KeepActorOnFloor(short SpriteNum);
int DoActorSlide(short SpriteNum);
int DoActorSectorDamage(short SpriteNum);
int DoScaleSprite(short SpriteNum);
END_SW_NS
