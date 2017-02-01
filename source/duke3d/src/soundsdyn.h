//-------------------------------------------------------------------------
/*
Copyright (C) 2013 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
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

#define DYNSOUNDREMAP_ENABLE


#define KICK_HIT__STATIC 0
#define PISTOL_RICOCHET__STATIC 1
#define PISTOL_BODYHIT__STATIC 2
#define PISTOL_FIRE__STATIC 3
#define EJECT_CLIP__STATIC 4
#define INSERT_CLIP__STATIC 5
#define CHAINGUN_FIRE__STATIC 6
#define RPG_SHOOT__STATIC 7
#define POOLBALLHIT__STATIC 8
#define RPG_EXPLODE__STATIC 9
#define CAT_FIRE__STATIC 10
#define SHRINKER_FIRE__STATIC 11
#define PIPEBOMB_BOUNCE__STATIC 13
#define PIPEBOMB_EXPLODE__STATIC 14
#define LASERTRIP_ONWALL__STATIC 15
#define LASERTRIP_ARMING__STATIC 16
#define LASERTRIP_EXPLODE__STATIC 17
#define VENT_BUST__STATIC 18
#define GLASS_BREAKING__STATIC 19
#define GLASS_HEAVYBREAK__STATIC 20
#define SHORT_CIRCUIT__STATIC 21
#define ITEM_SPLASH__STATIC 22
#define DUKE_GASP__STATIC 25
#define SLIM_RECOG__STATIC 26
#define DUKE_URINATE__STATIC 28
#define ENDSEQVOL3SND2__STATIC 29
#define ENDSEQVOL3SND3__STATIC 30
#define DUKE_CRACK__STATIC 33
#define SLIM_ATTACK__STATIC 34
#define SOMETHINGHITFORCE__STATIC 35
#define DUKE_DRINKING__STATIC 36
#define DUKE_GRUNT__STATIC 38
#define DUKE_HARTBEAT__STATIC 39
#define DUKE_ONWATER__STATIC 40
#define DUKE_LAND__STATIC 42
#define DUKE_WALKINDUCTS__STATIC 43
#define DUKE_UNDERWATER__STATIC 48
#define DUKE_JETPACK_ON__STATIC 49
#define DUKE_JETPACK_IDLE__STATIC 50
#define DUKE_JETPACK_OFF__STATIC 51
#define DUKETALKTOBOSS__STATIC 56
#define SQUISHED__STATIC 69
#define TELEPORTER__STATIC 70
#define ELEVATOR_ON__STATIC 71
#define ELEVATOR_OFF__STATIC 73
#define SUBWAY__STATIC 75
#define SWITCH_ON__STATIC 76
#define FLUSH_TOILET__STATIC 79
#define EARTHQUAKE__STATIC 81
#define END_OF_LEVEL_WARN__STATIC 83
#define WIND_AMBIENCE__STATIC 91
#define SOMETHING_DRIPPING__STATIC 92
#define BOS1_RECOG__STATIC 97
#define BOS2_RECOG__STATIC 102
#define DUKE_GETWEAPON2__STATIC 107
#define SHOTGUN_FIRE__STATIC 109
#define PRED_RECOG__STATIC 111
#define CAPT_RECOG__STATIC 117
#define PIG_RECOG__STATIC 121
#define RECO_ROAM__STATIC 125
#define RECO_RECOG__STATIC 126
#define RECO_ATTACK__STATIC 127
#define RECO_PAIN__STATIC 128
#define DRON_RECOG__STATIC 131
#define COMM_RECOG__STATIC 136
#define OCTA_RECOG__STATIC 141
#define TURR_RECOG__STATIC 146
#define SLIM_DYING__STATIC 149
#define BOS3_RECOG__STATIC 151
#define BOS1_WALK__STATIC 156
#define THUD__STATIC 158
#define WIERDSHOT_FLY__STATIC 160
#define SLIM_ROAM__STATIC 163
#define SHOTGUN_COCK__STATIC 169
#define GENERIC_AMBIENCE17__STATIC 177
#define BONUS_SPEECH1__STATIC 195
#define BONUS_SPEECH2__STATIC 196
#define BONUS_SPEECH3__STATIC 197
#define BONUS_SPEECH4__STATIC 199
#define DUKE_LAND_HURT__STATIC 200
#define DUKE_SEARCH2__STATIC 207
#define DUKE_CRACK2__STATIC 208
#define DUKE_SEARCH__STATIC 209
#define DUKE_GET__STATIC 210
#define DUKE_LONGTERM_PAIN__STATIC 211
#define MONITOR_ACTIVE__STATIC 212
#define NITEVISION_ONOFF__STATIC 213
#define DUKE_CRACK_FIRST__STATIC 215
#define DUKE_USEMEDKIT__STATIC 216
#define DUKE_TAKEPILLS__STATIC 217
#define DUKE_PISSRELIEF__STATIC 218
#define SELECT_WEAPON__STATIC 219
#define JIBBED_ACTOR5__STATIC 226
#define JIBBED_ACTOR6__STATIC 227
#define DUKE_GOTHEALTHATLOW__STATIC 229
#define BOSSTALKTODUKE__STATIC 230
#define WAR_AMBIENCE2__STATIC 232
#define EXITMENUSOUND__STATIC 243
#define FLY_BY__STATIC 244
#define DUKE_SCREAM__STATIC 245
#define SHRINKER_HIT__STATIC 246
#define RATTY__STATIC 247
#define BONUSMUSIC__STATIC 249
#define DUKE_GETWEAPON6__STATIC 264
#define ALIEN_SWITCH1__STATIC 272
#define RIPHEADNECK__STATIC 284
#define ENDSEQVOL3SND4__STATIC 288
#define ENDSEQVOL3SND5__STATIC 289
#define ENDSEQVOL3SND6__STATIC 290
#define ENDSEQVOL3SND7__STATIC 291
#define ENDSEQVOL3SND8__STATIC 292
#define ENDSEQVOL3SND9__STATIC 293
#define WHIPYOURASS__STATIC 294
#define ENDSEQVOL2SND1__STATIC 295
#define ENDSEQVOL2SND2__STATIC 296
#define ENDSEQVOL2SND3__STATIC 297
#define ENDSEQVOL2SND4__STATIC 298
#define ENDSEQVOL2SND5__STATIC 299
#define ENDSEQVOL2SND6__STATIC 300
#define ENDSEQVOL2SND7__STATIC 301
#define SOMETHINGFROZE__STATIC 303
#define WIND_REPEAT__STATIC 308
#define BOS4_RECOG__STATIC 342
#define LIGHTNING_SLAP__STATIC 351
#define THUNDER__STATIC 352
#define INTRO4_1__STATIC 363
#define INTRO4_2__STATIC 364
#define INTRO4_3__STATIC 365
#define INTRO4_4__STATIC 366
#define INTRO4_5__STATIC 367
#define INTRO4_6__STATIC 368
#define BOSS4_DEADSPEECH__STATIC 370
#define BOSS4_FIRSTSEE__STATIC 371
#define VOL4ENDSND1__STATIC 384
#define VOL4ENDSND2__STATIC 385
#define EXPANDERSHOOT__STATIC 388
#define INTRO4_B__STATIC 392
#define BIGBANG__STATIC 393

extern int16_t DynamicSoundMap[MAXSOUNDS];

void G_InitDynamicSounds(void);

#ifdef DYNSOUNDREMAP_ENABLE

void G_ProcessDynamicSoundMapping(const char *szLabel, int32_t lValue);

#if !defined LUNATIC
void initsoundhashnames(void);
void freesoundhashnames(void);
#endif

extern int32_t KICK_HIT;
extern int32_t PISTOL_RICOCHET;
extern int32_t PISTOL_BODYHIT;
extern int32_t PISTOL_FIRE;
extern int32_t EJECT_CLIP;
extern int32_t INSERT_CLIP;
extern int32_t CHAINGUN_FIRE;
extern int32_t RPG_SHOOT;
extern int32_t POOLBALLHIT;
extern int32_t RPG_EXPLODE;
extern int32_t CAT_FIRE;
extern int32_t SHRINKER_FIRE;
extern int32_t PIPEBOMB_BOUNCE;
extern int32_t PIPEBOMB_EXPLODE;
extern int32_t LASERTRIP_ONWALL;
extern int32_t LASERTRIP_ARMING;
extern int32_t LASERTRIP_EXPLODE;
extern int32_t VENT_BUST;
extern int32_t GLASS_BREAKING;
extern int32_t GLASS_HEAVYBREAK;
extern int32_t SHORT_CIRCUIT;
extern int32_t ITEM_SPLASH;
extern int32_t DUKE_GASP;
extern int32_t SLIM_RECOG;
extern int32_t DUKE_URINATE;
extern int32_t ENDSEQVOL3SND2;
extern int32_t ENDSEQVOL3SND3;
extern int32_t DUKE_CRACK;
extern int32_t SLIM_ATTACK;
extern int32_t SOMETHINGHITFORCE;
extern int32_t DUKE_DRINKING;
extern int32_t DUKE_GRUNT;
extern int32_t DUKE_HARTBEAT;
extern int32_t DUKE_ONWATER;
extern int32_t DUKE_LAND;
extern int32_t DUKE_WALKINDUCTS;
extern int32_t DUKE_UNDERWATER;
extern int32_t DUKE_JETPACK_ON;
extern int32_t DUKE_JETPACK_IDLE;
extern int32_t DUKE_JETPACK_OFF;
extern int32_t DUKETALKTOBOSS;
extern int32_t SQUISHED;
extern int32_t TELEPORTER;
extern int32_t ELEVATOR_ON;
extern int32_t ELEVATOR_OFF;
extern int32_t SUBWAY;
extern int32_t SWITCH_ON;
extern int32_t FLUSH_TOILET;
extern int32_t EARTHQUAKE;
extern int32_t END_OF_LEVEL_WARN;
extern int32_t WIND_AMBIENCE;
extern int32_t SOMETHING_DRIPPING;
extern int32_t BOS1_RECOG;
extern int32_t BOS2_RECOG;
extern int32_t DUKE_GETWEAPON2;
extern int32_t SHOTGUN_FIRE;
extern int32_t PRED_RECOG;
extern int32_t CAPT_RECOG;
extern int32_t PIG_RECOG;
extern int32_t RECO_ROAM;
extern int32_t RECO_RECOG;
extern int32_t RECO_ATTACK;
extern int32_t RECO_PAIN;
extern int32_t DRON_RECOG;
extern int32_t COMM_RECOG;
extern int32_t OCTA_RECOG;
extern int32_t TURR_RECOG;
extern int32_t SLIM_DYING;
extern int32_t BOS3_RECOG;
extern int32_t BOS1_WALK;
extern int32_t THUD;
extern int32_t WIERDSHOT_FLY;
extern int32_t SLIM_ROAM;
extern int32_t SHOTGUN_COCK;
extern int32_t GENERIC_AMBIENCE17;
extern int32_t BONUS_SPEECH1;
extern int32_t BONUS_SPEECH2;
extern int32_t BONUS_SPEECH3;
extern int32_t BONUS_SPEECH4;
extern int32_t DUKE_LAND_HURT;
extern int32_t DUKE_SEARCH2;
extern int32_t DUKE_CRACK2;
extern int32_t DUKE_SEARCH;
extern int32_t DUKE_GET;
extern int32_t DUKE_LONGTERM_PAIN;
extern int32_t MONITOR_ACTIVE;
extern int32_t NITEVISION_ONOFF;
extern int32_t DUKE_CRACK_FIRST;
extern int32_t DUKE_USEMEDKIT;
extern int32_t DUKE_TAKEPILLS;
extern int32_t DUKE_PISSRELIEF;
extern int32_t SELECT_WEAPON;
extern int32_t JIBBED_ACTOR5;
extern int32_t JIBBED_ACTOR6;
extern int32_t DUKE_GOTHEALTHATLOW;
extern int32_t BOSSTALKTODUKE;
extern int32_t WAR_AMBIENCE2;
extern int32_t EXITMENUSOUND;
extern int32_t FLY_BY;
extern int32_t DUKE_SCREAM;
extern int32_t SHRINKER_HIT;
extern int32_t RATTY;
extern int32_t BONUSMUSIC;
extern int32_t DUKE_GETWEAPON6;
extern int32_t ALIEN_SWITCH1;
extern int32_t RIPHEADNECK;
extern int32_t ENDSEQVOL3SND4;
extern int32_t ENDSEQVOL3SND5;
extern int32_t ENDSEQVOL3SND6;
extern int32_t ENDSEQVOL3SND7;
extern int32_t ENDSEQVOL3SND8;
extern int32_t ENDSEQVOL3SND9;
extern int32_t WHIPYOURASS;
extern int32_t ENDSEQVOL2SND1;
extern int32_t ENDSEQVOL2SND2;
extern int32_t ENDSEQVOL2SND3;
extern int32_t ENDSEQVOL2SND4;
extern int32_t ENDSEQVOL2SND5;
extern int32_t ENDSEQVOL2SND6;
extern int32_t ENDSEQVOL2SND7;
extern int32_t SOMETHINGFROZE;
extern int32_t WIND_REPEAT;
extern int32_t BOS4_RECOG;
extern int32_t LIGHTNING_SLAP;
extern int32_t THUNDER;
extern int32_t INTRO4_1;
extern int32_t INTRO4_2;
extern int32_t INTRO4_3;
extern int32_t INTRO4_4;
extern int32_t INTRO4_5;
extern int32_t INTRO4_6;
extern int32_t BOSS4_DEADSPEECH;
extern int32_t BOSS4_FIRSTSEE;
extern int32_t VOL4ENDSND1;
extern int32_t VOL4ENDSND2;
extern int32_t EXPANDERSHOOT;
extern int32_t INTRO4_B;
extern int32_t BIGBANG;

#define DYNAMICSOUNDMAP(Soundnum) (DynamicSoundMap[Soundnum])

#else  /* if !defined DYNSOUNDREMAP_ENABLE */

#define G_ProcessDynamicSoundMapping(x, y) ((void)(0))

#define initsoundhashnames() ((void)0)
#define freesoundhashnames() ((void)0)

#include "soundefs.h"

#define DYNAMICSOUNDMAP(Soundnum) (Soundnum)

#endif
