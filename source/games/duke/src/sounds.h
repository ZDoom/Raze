//****************************************************************************
//
// sounds.h
//
//****************************************************************************
#pragma once

#include "raze_sound.h"
#include "raze_music.h"
struct MapRecord;

BEGIN_DUKE_NS

// Sound flags
enum {
	SF_LOOP = 1,
	SF_MSFX = 2,
	SF_TALK = 4,
	SF_ADULT = 8,
	SF_GLOBAL = 16,
	SF_ONEINST_INTERNAL = 32,

	SF_DTAG = 128,
};

enum esound_t
{
	kPitchStart,
	kPitchEnd,
	kVolAdjust,
	kPriority,
	kFlags,
	kWorldTourMapping,
	kMaxUserData
};

void S_InitSound();
void S_CacheAllSounds(void);
int S_DefineSound(unsigned index, const char* filename, int ps, int pe, int pr, int m, int vo, float vol);
void S_WorldTourMappingsForOldSounds();

int S_PlaySound(int num, int channel = CHAN_AUTO, EChanFlags flags = 0, float vol =0.8f);
int S_PlaySound3D(int num, DDukeActor* spriteNum, const vec3_t* pos, int channel = CHAN_AUTO, EChanFlags flags = 0);
int S_PlayActorSound(int soundNum, DDukeActor* spriteNum, int channel = CHAN_AUTO, EChanFlags flags = 0);
void S_MenuSound(void);

void S_StopSound(int sndNum, DDukeActor* spr = nullptr, int flags = -1);

int S_CheckSoundPlaying(int soundNum);
int S_CheckActorSoundPlaying(DDukeActor* spriteNum, int soundNum, int channel = 0);
int S_CheckAnyActorSoundPlaying(DDukeActor* spriteNum);

void S_ChangeSoundPitch(int soundNum, DDukeActor* spriteNum, int pitchoffset);
int S_GetUserFlags(int sndnum);

inline bool S_IsSoundValid(int num)
{
	return (!soundEngine->isValidSoundId(num + 1));
}


void S_PlayRRMusic(int newTrack = -1);
void S_PlayBonusMusic();
void S_PlayLevelMusic(MapRecord* mi);
void S_ContinueLevelMusic(void);

// Placeholders.

void S_ParseDeveloperCommentary();

void StopCommentary();
bool StartCommentary(int tag, DDukeActor* sprnum);


END_DUKE_NS

