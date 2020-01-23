#pragma once

BEGIN_SW_NS

void KeysCheat(PLAYERp pp, const char *cheat_string);
void ResCheatOn(PLAYERp, const char *);
void VoxCheat(PLAYERp, const char *);
void RestartCheat(PLAYERp, const char *);
void RoomCheat(PLAYERp, const char *);
void SecretCheat(PLAYERp pp, const char *);
void NextCheat(PLAYERp pp, const char *);
void PrevCheat(PLAYERp pp, const char *);
void MapCheat(PLAYERp pp, const char *);
void LocCheat(PLAYERp pp, const char *);
void GunsCheat(PLAYERp pp, const char *cheat_string);
void WeaponCheat(PLAYERp pp, const char *);
void GodCheat(PLAYERp pp, const char *);
void ClipCheat(PLAYERp pp, const char *);
void WarpCheat(PLAYERp pp, const char *cheat_string);
void ItemCheat(PLAYERp pp, const char *cheat_string);
void HealCheat(PLAYERp pp, const char *cheat_string);
void SortKeyCheat(PLAYERp pp, const char *sKey);
void KeysCheat(PLAYERp pp, const char *cheat_string);
void EveryCheatToggle(PLAYERp pp, const char *cheat_string);
void GeorgeFunc(PLAYERp pp, char *);
void BlackburnFunc(PLAYERp pp, char *);
int cheatcmp(const char *str1, const char *str2, int len);
void CheatInput(void);

END_SW_NS
