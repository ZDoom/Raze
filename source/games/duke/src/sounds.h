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
	SF_CONDEFINED = 64,

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

int S_PlaySound(FSoundID num, int channel = CHAN_AUTO, EChanFlags flags = 0, float vol = 0.8f);
int S_PlaySound3D(FSoundID num, DDukeActor* spriteNum, const DVector3& pos, int channel = CHAN_AUTO, EChanFlags flags = 0);
int S_PlayActorSound(FSoundID soundNum, DDukeActor* spriteNum, int channel = CHAN_AUTO, EChanFlags flags = 0);
void S_StopSound(FSoundID sndNum, DDukeActor* spr = nullptr, int flags = -1);
void S_ChangeSoundPitch(FSoundID soundNum, DDukeActor* spriteNum, int pitchoffset);
int S_CheckActorSoundPlaying(DDukeActor* spriteNum, FSoundID soundNum, int channel = 0);
int S_CheckSoundPlaying(FSoundID soundNum);

void S_CacheAllSounds(void);
int S_DefineSound(unsigned index, const char* filename, int ps, int pe, int pr, int m, int vo, float vol);
void S_WorldTourMappingsForOldSounds();
void S_MenuSound(void);


int S_CheckAnyActorSoundPlaying(DDukeActor* spriteNum);

int S_GetUserFlags(FSoundID sndnum);

inline bool S_IsSoundValid(int num)
{
	return !soundEngine->isValidSoundId(S_FindSoundByResID(num));
}

inline int S_PlaySound(int num, int channel = CHAN_AUTO, EChanFlags flags = 0, float vol = 0.8f)
{
	return S_PlaySound(S_FindSoundByResID(num), channel, flags, vol);
}

inline int S_PlaySound3D(int num, DDukeActor* spriteNum, const DVector3& pos, int channel = CHAN_AUTO, EChanFlags flags = 0)
{
	return S_PlaySound3D(S_FindSoundByResID(num), spriteNum, pos, channel, flags);
}

inline int S_PlayActorSound(int soundNum, DDukeActor* spriteNum, int channel = CHAN_AUTO, EChanFlags flags = 0)
{
	return S_PlayActorSound(S_FindSoundByResID(soundNum), spriteNum, channel, flags);
}

inline void S_StopSound(int sndNum, DDukeActor* spr = nullptr, int flags = -1)
{
	return S_StopSound(S_FindSoundByResID(sndNum), spr, flags);
}

inline void S_ChangeSoundPitch(int soundNum, DDukeActor* spriteNum, int pitchoffset)
{
	S_ChangeSoundPitch(S_FindSoundByResID(soundNum), spriteNum, pitchoffset);
}

inline int S_CheckActorSoundPlaying(DDukeActor* spriteNum, int soundNum, int channel = 0)
{
	return S_CheckActorSoundPlaying(spriteNum, S_FindSoundByResID(soundNum), channel);
}

inline int S_CheckSoundPlaying(int soundNum)
{
	return S_CheckSoundPlaying(S_FindSoundByResID(soundNum));
}

inline int S_GetUserFlags(int soundNum)
{
	return S_GetUserFlags(S_FindSoundByResID(soundNum));
}


void S_PlayRRMusic(int newTrack = -1);
void S_PlayBonusMusic();
void S_PlayLevelMusic(MapRecord* mi);
void S_ContinueLevelMusic(void);

// Placeholders.

void S_ParseDeveloperCommentary();

void StopCommentary();
int StartCommentary(int tag, DDukeActor* sprnum);


END_DUKE_NS

