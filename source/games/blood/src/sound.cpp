//-------------------------------------------------------------------------
/*
Copyright (C) 2020-2022 Christoph Oelckers

This file is part of Raze

Raze is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

*/
//-------------------------------------------------------------------------

#include "ns.h"	// Must come before everything else!

#include "build.h"
#include "blood.h"
#include "raze_music.h"
#include "raze_sound.h"
#include "gamecontrol.h"

BEGIN_BLD_NS

int soundRates[13] = {
	11025,
	11025,
	11025,
	11025,
	11025,
	22050,
	22050,
	22050,
	22050,
	44100,
	44100,
	44100,
	44100,
};


void ByteSwapSFX(SFX* pSFX)
{
#if WORDS_BIGENDIAN
	pSFX->relVol = LittleLong(pSFX->relVol);
	pSFX->pitch = LittleLong(pSFX->pitch);
	pSFX->pitchRange = LittleLong(pSFX->pitchRange);
	pSFX->format = LittleLong(pSFX->format);
	pSFX->loopStart = LittleLong(pSFX->loopStart);
#endif
}

//==========================================================================
//
// S_AddBloodSFX
//
//==========================================================================

static void S_AddBloodSFX(int lumpnum)
{
	FSoundID sfxnum;

	int resid = fileSystem.GetResourceId(lumpnum);
	sfxinfo_t* soundfx = nullptr;

	sfxnum = soundEngine->FindSoundByResIDNoHash(resid);
	if (sfxnum.isvalid())
	{
		soundfx = soundEngine->GetWritableSfx(sfxnum);
		if (!soundfx->bTentative) return;	// sound was already defined.
	}

	auto sfxlump = fileSystem.ReadFile(lumpnum);
	SFX* sfx = (SFX*)sfxlump.GetMem();
	ByteSwapSFX(sfx);

	FStringf rawname("%s.raw", sfx->rawName);
	auto rawlump = fileSystem.FindFile(rawname.GetChars());

	if (rawlump != -1)
	{
		if (!sfxnum.isvalid())
		{
			sfxnum = soundEngine->AddSoundLump(FStringf("SfxSound@%04d", resid).GetChars(), rawlump, 0, resid, 6);	// use a generic name here in case sound replacements are being used.
			soundfx = soundEngine->GetWritableSfx(sfxnum);
		}
		if (sfx->format < 5 || sfx->format > 12)
		{	// [0..4] + invalid formats
			soundfx->RawRate = 11025;
		}
		else if (sfx->format < 9)
		{	// [5..8]
			soundfx->RawRate = 22050;
		}
		else
		{	// [9..12]
			soundfx->RawRate = 44100;
		}
		soundfx->NearLimit = 6;
		soundfx->lumpnum = rawlump;
		soundfx->bLoadRAW = true;
		soundfx->bExternal = true;
		soundfx->bTentative = false;
		soundfx->LoopStart = LittleLong(sfx->loopStart);
		//S_sfx[sfxnum].Volume = sfx->relVol / 255.f; This cannot be done because this volume setting is optional.
		// pitchrange is unused.
		if (sfx->pitch != 0x10000) soundfx->DefPitch = sfx->pitch / 65536.f;
		else soundfx->DefPitch = 0;
		if (sfx->relVol != 80) // 80 is the default
		{
			soundfx->UserVal = sfx->relVol;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void GameInterface::StartSoundEngine()
{
	soundEngine = new BloodSoundEngine;
}

void sndInit(void)
{
	soundEngine->AddSoundLump("", 0, 0, -1, 6); // add a dummy entry at index #0
	for (int i = fileSystem.GetNumEntries() - 1; i >= 0; i--)
	{
		auto type = fileSystem.GetResourceType(i);
		if (!stricmp(type, "SFX"))
		{
			S_AddBloodSFX(i);
		}
		else if (!stricmp(type, "WAV") || !stricmp(type, "OGG") || !stricmp(type, "FLAC") || !stricmp(type, "VOC"))
		{
			if (fileSystem.GetFileNamespace(i) != FileSys::ns_music)
				soundEngine->AddSoundLump(fileSystem.GetFileFullName(i), i, 0, fileSystem.GetResourceId(i) | 0x40000000, 6); // mark the resource ID as special.
		}
	}
	soundEngine->HashSounds();
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int sndGetRate(int format)
{
	if (format < 13)
		return soundRates[format];
	return 11025;
}

bool sndCheckPlaying(unsigned int nSound)
{
	auto snd = soundEngine->FindSoundByResID(nSound);
	return snd.isvalid() ? soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, snd) : false;
}

void sndStopSample(unsigned int nSound)
{
	auto snd = soundEngine->FindSoundByResID(nSound);

	if (snd.isvalid())
	{
		soundEngine->StopSoundID(snd);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void sndStartSample(const char* pzSound, int nVolume, int nChannel)
{
	if (!SoundEnabled())
		return;
	if (!strlen(pzSound))
		return;
	auto snd = soundEngine->FindSound(pzSound);
	if (snd.isvalid())
	{
		soundEngine->StartSound(SOURCE_None, nullptr, nullptr, nChannel + 1, 0, snd, nVolume / 255.f, ATTN_NONE);
	}
}

void sndStartSample(unsigned int nSound, int nVolume, int nChannel, bool bLoop, EChanFlags chanflags)
{
	if (!SoundEnabled())
		return;
	if (nChannel >= 7) nChannel = -1;
	auto snd = soundEngine->FindSoundByResID(nSound);
	if (snd.isvalid())
	{
		if (nVolume < 0)
		{
			auto udata = soundEngine->GetSfx(snd);
			if (udata) nVolume = min(Scale(udata->UserVal, 255, 80), 255);
			else nVolume = 255;
		}
		if (bLoop) chanflags |= CHANF_LOOP;
		soundEngine->StopActorSounds(SOURCE_None, nullptr, nChannel + 1, nChannel + 1);
		soundEngine->StartSound(SOURCE_None, nullptr, nullptr, (nChannel + 1), chanflags, snd, nVolume / 255.f, ATTN_NONE);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void sndStartWavID(unsigned int nSound, int nVolume, int nChannel)
{
	return sndStartSample(nSound | 0x40000000, nVolume, nChannel);
}

void sndStartWavDisk(const char* pzFile, int nVolume, int nChannel)
{
	FString name = pzFile;
	FixPathSeperator(name);
	return sndStartSample(name.GetChars(), nVolume, nChannel);
}

void sndKillAllSounds(void)
{
	if (soundEngine) soundEngine->StopSound(CHAN_AUTO);
}



END_BLD_NS
