#pragma once

#include "misc.h"
#include "raze_music.h"
#include "s_soundinternal.h"

BEGIN_BLD_NS

struct SFX
{
	int relVol;
	int pitch;
	int pitchRange;
	int format;
	int loopStart;
	char rawName[9];
};

int sndGetRate(int format);
bool sndCheckPlaying(unsigned int nSound);
void sndStopSample(unsigned int nSound);
void sndStartSample(const char* pzSound, int nVolume, int nChannel = -1);
void sndStartSample(unsigned int nSound, int nVolume, int nChannel = -1, bool bLoop = false, EChanFlags soundflags = CHANF_NONE);
void sndStartWavID(unsigned int nSound, int nVolume, int nChannel = -1);
void sndStartWavDisk(const char* pzFile, int nVolume, int nChannel = -1);
void sndKillAllSounds(void);
void sndProcess(void);
void sndTerm(void);
void sndInit(void);

void sfxPlay3DSound(const DVector3& pos, int soundId, sectortype* pSector);
void sfxPlay3DSound(DBloodActor* pSprite, int soundId, int a3 = -1, int a4 = 0);
void sfxPlay3DSoundVolume(DBloodActor* pSprite, int soundId, int a3 = -1, int a4 = 0, int pitch = 0, int volume = 0);
void sfxKill3DSound(DBloodActor* pSprite, int a2 = -1, int a3 = -1);
void sfxKillAllSounds(void);
void sfxSetReverb(bool toggle);
void sfxSetReverb2(bool toggle);

void ambProcess(PLAYER* pPlayer);
void ambKillAll(void);
void ambInit(void);

enum EPlayFlags
{
	FX_GlobalChannel = 1,
	FX_SoundMatch = 2,
	FX_ChannelMatch = 4,
};


END_BLD_NS
