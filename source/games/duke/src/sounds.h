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

int A_CheckSoundPlaying(int spriteNum, int soundNum, int channel = 0);
int A_PlaySound(int soundNum, int spriteNum, int channel = CHAN_AUTO, EChanFlags flags = 0);
inline int spritesound(int soundnum, int spritenum)
{
    return A_PlaySound(soundnum, spritenum);
}
int A_CheckAnySoundPlaying(int spriteNum);
int S_CheckSoundPlaying(int soundNum);
inline int S_CheckSoundPlaying(int sprnum, int soundNum) { return S_CheckSoundPlaying(soundNum); }
void cacheAllSounds(void);
void S_MenuSound(void);
void S_PauseMusic(bool paused);
void S_PauseSounds(bool paused);
void S_PlayLevelMusic(MapRecord* mi);
void S_PlaySpecialMusic(unsigned int);
void S_ContinueLevelMusic(void);
int S_PlaySound(int num, int channel = CHAN_AUTO, EChanFlags flags = 0);
inline int sound(int num)
{
    return S_PlaySound(num);
}
int S_PlaySound3D(int num, int spriteNum, const vec3_t *pos, int channel = CHAN_AUTO, EChanFlags flags = 0);
void S_StopEnvSound(int sndNum,int sprNum, int flags = -1);
inline void stopsound(int snd)
{
    S_StopEnvSound(snd, -1);
}
#define S_StopSound(num) S_StopEnvSound(num, -1)

void S_Update(void);
void S_ChangeSoundPitch(int soundNum, int spriteNum, int pitchoffset);
int S_GetUserFlags(int sndnum);
int S_DefineSound(unsigned index, const char* filename, int ps, int pe, int pr, int m, int vo, float vol);
void S_InitSound();
void S_PlayRRMusic(int newTrack = -1);
void PlayBonusMusic();

inline bool S_IsSoundValid(int num)
{
    return (!soundEngine->isValidSoundId(num + 1));

}

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
