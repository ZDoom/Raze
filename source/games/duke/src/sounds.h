//****************************************************************************
//
// sounds.h
//
//****************************************************************************

#ifndef sounds_public_h_
#define sounds_public_h_

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
	kMaxUserData
};

void S_InitSound();
void S_Update(void);
void S_CacheAllSounds(void);
int S_DefineSound(unsigned index, const char* filename, int ps, int pe, int pr, int m, int vo, float vol);

int S_PlaySound(int num, int channel = CHAN_AUTO, EChanFlags flags = 0, float vol =0.8f);
int S_PlaySound3D(int num, int spriteNum, const vec3_t* pos, int channel = CHAN_AUTO, EChanFlags flags = 0);
int S_PlayActorSound(int soundNum, int spriteNum, int channel = CHAN_AUTO, EChanFlags flags = 0);
void S_MenuSound(void);

void S_StopSound(int sndNum, int sprNum = -1, int flags = -1);

int S_CheckSoundPlaying(int soundNum);
inline int S_CheckSoundPlaying(int sprnum, int soundNum) { return S_CheckSoundPlaying(soundNum); }
int S_CheckActorSoundPlaying(int spriteNum, int soundNum, int channel = 0);
int S_CheckAnyActorSoundPlaying(int spriteNum);

void S_ChangeSoundPitch(int soundNum, int spriteNum, int pitchoffset);
int S_GetUserFlags(int sndnum);

inline bool S_IsSoundValid(int num)
{
	return (!soundEngine->isValidSoundId(num + 1));
}


void S_PlayRRMusic(int newTrack = -1);
void S_PlayBonusMusic();
void S_PlayLevelMusic(MapRecord* mi);
void S_PlaySpecialMusic(unsigned int);
void S_ContinueLevelMusic(void);

// Placeholders.
inline void StopCommentary()
{}

inline bool StartCommentary(int tag, int sprnum)
{
	return false;
}

extern TArray<FString> specialmusic;


END_DUKE_NS

#endif
