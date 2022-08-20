//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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

#pragma once 
#include "raze_sound.h"

BEGIN_PS_NS


enum
{
	kMaxSoundFiles      = 80,
	kMaxSounds          = 200,
	kMaxSoundNameLen    = 8,
	kMaxActiveSounds    = 8,
	kCreepyCount 		= 150,
	MUSIC_ID    		= (-65536)
};

enum {
    kSound0 = 0,
    kSound1,
    kSound2,
    kSound3,
    kSound4,
    kSound5,
    kSound6,
    kSound7,
    kSound8,
    kSound9,
    kSoundItemSpecial,
    kSound11,
    kSoundTorchOn,
    kSound13,
    kSound14,
    kSound15,
    kSound16,
    kSound17,
    kSound18,
    kSound19,
    kSound20,
    kSound21,
    kSound22,
    kSound23,
    kSound24,
    kSound25,
    kSound26,
    kSound27,
    kSoundJonLaugh2,
    kSound29,
    kSound30,
    kSound31,
    kSound32,
    kSound33,
    kSound34,
    kSound35,
    kSound36,
    kSound38 = 38,
    kSound39,
    kSound40,
    kSound41,
    kSound42,
    kSound43,
    kSound47 = 47,
    kSound48 = 48,
    kSoundQTail = 50,
    kSound52 = 52,
    kSoundTauntStart = 53,
    kSoundJonFDie = 60,
    kSound61,
    kSound62,
    kSound63,
    kSound64,
    kSound65,
    kSound66,
    kSoundMana1,
    kSoundMana2,
    kSoundAmmoPickup,
    kSound70,
    kSound71,
    kSound72,
    kSoundAlarm,
    kSound74,
    kSound75,
    kSound76,
    kSound77,
    kSound78,
    kSound79,
};

extern int gMusicVolume;
extern int gFXVolume;
extern int nStopSound;
extern int nStoneSound;
extern int nSwitchSound;
extern sectortype* pLocalEyeSect;
extern int nElevSound;
extern int nCreepyTimer;

extern int16_t StaticSound[];


void UpdateCreepySounds();

void InitFX();
void FadeSong();
int fadecdaudio();
int LocalSoundPlaying();
void LoadFX();
void StopAllSounds();
void StopLocalSound();
void PlayLocalSound(int nSound, int val, bool unattached = false, EChanFlags cflags = CHANF_NONE);
int LoadSound(const char* sound);

void BendAmbientSound();
void CheckAmbience(sectortype* pSector);

void PlayFX2(int nSound, DExhumedActor* nSprite, int sectf = 0, EChanFlags chanflags = CHANF_NONE, int sprflags = 0, const DVector3* soundpos = nullptr);

void PlayFXAtXYZ(int nSound, const DVector3& pos, EChanFlags chanflags = CHANF_NONE, int sectf = 0);
inline void D3PlayFX(int nSound, DExhumedActor* actor, int flags = 0)
{
    PlayFX2(nSound, actor, 0, CHANF_NONE, flags);
}

void StopActorSound(DExhumedActor* actor);

void StartSwirlies();
void UpdateSwirlies();

void PlayTitleSound(void);
void PlayGameOverSound(void);

void SoundBigEntrance(void);

END_PS_NS
