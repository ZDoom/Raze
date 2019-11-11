
#include "compat.h"

#include "../../audiolib/src/_multivc.h"
#include "multivoc.h"
#include "../../audiolib/src/pitch.h"
#include "pragmas.h"
#include "zmusic/zmusic.h"
#include "s_music.h"
#include "templates.h"
#include "fx_man.h"
#include "gamecvars.h"

static short buffer[2][16384];
static float readbuffer[16384];
static int whichbuffer;
static bool StreamPaused;
static int StreamHandle;
static VoiceNode* voice;

static bool FillStream(void* buff, int len)
{
	if (StreamPaused)
	{
		memset((char*)buff, 0, len);
		return true;
	}

	bool written = ZMusic_FillStream(mus_playing.handle, buff, len);
	
	if (!written)
	{
		memset((char*)buff, 0, len);
		return false;
	}
	return true;
}

 
static playbackstatus MV_GetNextZMusicBlock(VoiceNode *voice)
{
	if (!FillStream(readbuffer, 32768))
		return NoMoreData;
	
	for (int i = 0; i <16384; i++)
	{
		buffer[whichbuffer][i] = (short)clamp(readbuffer[i]*32767., -32768., 32767.);
	}

    voice->sound        = (const char*)buffer[whichbuffer];
    voice->length       = 8192 << 16;
    voice->position     = 0;
    voice->BlockLength  = 0;
	whichbuffer ^= 1;

    MV_SetVoiceMixMode(voice);

    return KeepPlaying;
}

void S_CreateStream()
{
	if (!MV_Installed)
		return;// MV_SetErrorCode(MV_NotInstalled);

    // Request a voice from the voice pool
    voice = MV_AllocVoice(FX_MUSIC_PRIORITY);
    if (voice == nullptr)
    {
		return;// MV_SetErrorCode(MV_NoVoices);
    }

    voice->length      = 0;
    voice->sound       = 0;

    voice->wavetype    = FMT_ZMUSIC;
    voice->rawdataptr  = nullptr;
    voice->GetSound    = MV_GetNextZMusicBlock;
    voice->LoopCount   = 0;
    voice->BlockLength = 0;
    voice->PitchScale  = PITCH_GetScale(0);
    voice->next        = nullptr;
    voice->prev        = nullptr;
    voice->priority    = FX_MUSIC_PRIORITY;
    voice->callbackval = 0;

    voice->bits        = 16;
    voice->channels    = 2;
    voice->SamplingRate = MV_MixRate;

    voice->Paused      = FALSE;

    voice->LoopStart   = 0;
    voice->LoopEnd     = 0;
    voice->LoopSize    = 1;

    // CODEDUP multivoc.c MV_SetVoicePitch
    voice->RateScale = divideu32(voice->SamplingRate * voice->PitchScale, MV_MixRate);
    voice->FixedPointBufferSize = (voice->RateScale * MV_MIXBUFFERSIZE) - voice->RateScale;
    MV_SetVoiceMixMode(voice);

	mus_volume.Callback();
    MV_PlayVoice(voice);

	return;// voice->handle;
}

void MV_ReleaseZMusicVoice(VoiceNode * voice)
{
    if (voice->wavetype != FMT_ZMUSIC)
        return;

    voice->rawdataptr = 0;
    voice->length = 0;
    voice->sound = nullptr;
	StreamHandle = 0;
	::voice = nullptr;
}

void S_PauseStream(bool pause)
{
	StreamPaused = true;
}

void S_StopStream()
{
	if (StreamHandle > 0)
		FX_StopSound(StreamHandle);
	StreamHandle = 0;
	voice = nullptr;
}

void S_SetStreamVolume(float vol)
{
	if (voice)
	MV_SetVoiceVolume(voice, int(vol * 255), int(vol * 255), int(vol * 255), 1);
}
