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

END_SW_NS
