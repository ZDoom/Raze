
/**
 * PlayStation XA (ADPCM) source support for MultiVoc
 * Adapted and remixed from superxa2wav
 */

#include "compat.h"
#include "pitch.h"
#include "multivoc.h"
#include "_multivc.h"

//#define NO_XA_HEADER

#define USE_FXD 1

#define kNumOfSamples   224
#define kNumOfSGs       18
#define TTYWidth		80

/* ADPCM */
#define XA_DATA_START   (0x44-48)

#define FXD_FxdToPCM(dt)        (max(min((short)((dt)>>16), 32767), -32768))
#define DblToPCM(dt)            (short)(max(min((dt), 32767), -32768))

#if USE_FXD
#define FXD_FxdToPcm16(dt)      (max(min((dt)/2, 32767), -32768))
#define FXD_Pcm16ToFxd(dt)      ((int32_t)dt*2)
#endif

typedef struct {
   void * ptr;
   size_t length;
   size_t pos;

#if USE_FXD
   int32_t t1, t2;
   int32_t t1_x, t2_x;
#else
   double t1, t2;
   double t1_x, t2_x;
#endif

   char *block;
   size_t blocksize;

   VoiceNode *owner;
} xa_data;

typedef char SoundGroup[128];

typedef struct XASector {
	char            sectorFiller[48];
	SoundGroup      SoundGroups[18];
} XASector;

#if USE_FXD
static int32_t      K0[4] = {
	0x00000000,
	0x0000F000,
	0x0001CC00,
	0x00018800,
};
static int32_t      K1[4] = {
	0x00000000,
	0x00000000,
	-53248, // 0xFFFF3000,
	-56320, // 0xFFFF2400,
};
#else
static double   K0[4] = {
	0.0,
	0.9375,
	1.796875,
	1.53125
};
static double   K1[4] = {
	0.0,
	0.0,
	-0.8125,
	-0.859375
};
#endif


#if USE_FXD
static int32_t FXD_FixMul(int32_t a, int32_t b)
{
	int32_t                high_a, low_a, high_b, low_b;
	int32_t                hahb, halb, lahb;
	uint32_t       lalb;
	int32_t                 ret;

	high_a = a >> 16;
	low_a = a & 0x0000FFFF;
	high_b = b >> 16;
	low_b = b & 0x0000FFFF;

	hahb = (high_a * high_b) << 16;
	halb = high_a * low_b;
	lahb = low_a * high_b;
	lalb = (uint32_t)(low_a * low_b) >> 16;

	ret = hahb + halb + lahb + lalb;

	return ret;
}
#endif



static int8_t getSoundData(char *buf, int32_t unit, int32_t sample)
{
	int8_t ret;
	char *p;
	int32_t offset, shift;

	p = buf;
	shift = (unit%2) * 4;

	offset = 16 + (unit / 2) + (sample * 4);
	p += offset;

	ret = (*p >> shift) & 0x0F;

	if (ret > 7) {
		ret -= 16;
	}
	return ret;
}

static int8_t getFilter(char *buf, int32_t unit)
{
	return (*(buf + 4 + unit) >> 4) & 0x03;
}


static int8_t getRange(char *buf, int32_t unit)
{
	return *(buf + 4 + unit) & 0x0F;
}


static void decodeSoundSectMono(XASector *ssct, xa_data * xad)
{
	size_t count = 0;
	int8_t snddat, filt, range;
	short decoded;
	int32_t unit, sample;
	int32_t sndgrp;
#if USE_FXD
	int32_t tmp2, tmp3, tmp4, tmp5;
#else
	double tmp2, tmp3, tmp4, tmp5;
#endif
    char decodeBuf[kNumOfSGs*kNumOfSamples*2];

	for (sndgrp = 0; sndgrp < kNumOfSGs; sndgrp++)
	{
		for (unit = 0; unit < 8; unit++)
		{
			range = getRange(ssct->SoundGroups[sndgrp], unit);
			filt = getFilter(ssct->SoundGroups[sndgrp], unit);
			for (sample = 0; sample < 28; sample++)
			{
				snddat = getSoundData(ssct->SoundGroups[sndgrp], unit, sample);
#if USE_FXD
				tmp2 = (int32_t)(snddat) << (12 - range);
				tmp3 = FXD_Pcm16ToFxd(tmp2);
				tmp4 = FXD_FixMul(K0[filt], xad->t1);
				tmp5 = FXD_FixMul(K1[filt], xad->t2);
				xad->t2 = xad->t1;
				xad->t1 = tmp3 + tmp4 + tmp5;
				decoded = FXD_FxdToPcm16(xad->t1);
#else
				tmp2 = (double)(1 << (12 - range));
				tmp3 = (double)snddat * tmp2;
				tmp4 = xad->t1 * K0[filt];
				tmp5 = xad->t2 * K1[filt];
				xad->t2 = xad->t1;
				xad->t1 = tmp3 + tmp4 + tmp5;
				decoded = DblToPCM(xad->t1);
#endif
				decodeBuf[count++] = (char)(decoded & 0x0000ffff);
				decodeBuf[count++] = (char)(decoded >> 8);
			}
		}
	}

    if (count > xad->blocksize)
        xad->block = (char*)realloc(xad->block, count);

    memcpy(xad->block, decodeBuf, count);
    xad->blocksize = count;
}

static void decodeSoundSectStereo(XASector *ssct, xa_data * xad)
{
	size_t count = 0;
	int8_t snddat, filt, range;
	int8_t filt1, range1;
	short decoded;
	int32_t unit, sample;
	int32_t sndgrp;
#if USE_FXD
	int32_t tmp2, tmp3, tmp4, tmp5;
#else
	double tmp2, tmp3, tmp4, tmp5;
#endif
    char decodeBuf[kNumOfSGs*kNumOfSamples*2];

	for (sndgrp = 0; sndgrp < kNumOfSGs; sndgrp++)
	{
		for (unit = 0; unit < 8; unit+= 2)
		{
			range = getRange(ssct->SoundGroups[sndgrp], unit);
			filt = getFilter(ssct->SoundGroups[sndgrp], unit);
			range1 = getRange(ssct->SoundGroups[sndgrp], unit+1);
			filt1 = getFilter(ssct->SoundGroups[sndgrp], unit+1);

			for (sample = 0; sample < 28; sample++)
			{
				// Channel 1
				snddat = getSoundData(ssct->SoundGroups[sndgrp], unit, sample);
#if USE_FXD
				tmp2 = (int32_t)(snddat) << (12 - range);
				tmp3 = FXD_Pcm16ToFxd(tmp2);
				tmp4 = FXD_FixMul(K0[filt], xad->t1);
				tmp5 = FXD_FixMul(K1[filt], xad->t2);
				xad->t2 = xad->t1;
				xad->t1 = tmp3 + tmp4 + tmp5;
				decoded = FXD_FxdToPcm16(xad->t1);
#else
				tmp2 = (double)(1 << (12 - range));
				tmp3 = (double)snddat * tmp2;
				tmp4 = xad->t1 * K0[filt];
				tmp5 = xad->t2 * K1[filt];
				xad->t2 = xad->t1;
				xad->t1 = tmp3 + tmp4 + tmp5;
				decoded = DblToPCM(xad->t1);
#endif
				decodeBuf[count++] = (char)(decoded & 0x0000ffff);
				decodeBuf[count++] = (char)(decoded >> 8);

				// Channel 2
				snddat = getSoundData(ssct->SoundGroups[sndgrp], unit+1, sample);
#if USE_FXD
				tmp2 = (int32_t)(snddat) << (12 - range1);
				tmp3 = FXD_Pcm16ToFxd(tmp2);
				tmp4 = FXD_FixMul(K0[filt1], xad->t1_x);
				tmp5 = FXD_FixMul(K1[filt1], xad->t2_x);
				xad->t2_x = xad->t1_x;
				xad->t1_x = tmp3 + tmp4 + tmp5;
				decoded = FXD_FxdToPcm16(xad->t1_x);
#else
				tmp2 = (double)(1 << (12 - range1));
				tmp3 = (double)snddat * tmp2;
				tmp4 = xad->t1_x * K0[filt1];
				tmp5 = xad->t2_x * K1[filt1];
				xad->t2_x = xad->t1_x;
				xad->t1_x = tmp3 + tmp4 + tmp5;
				decoded = DblToPCM(xad->t1_x);
#endif
				decodeBuf[count++] = (char)(decoded & 0x0000ffff);
				decodeBuf[count++] = (char)(decoded >> 8);
			}
		}
	}

    if (count > xad->blocksize)
        xad->block = (char*)realloc(xad->block, count);

    memcpy(xad->block, decodeBuf, count);
    xad->blocksize = count;
}

int32_t MV_GetXAPosition(VoiceNode *voice)
{
    xa_data * xad = (xa_data *) voice->rawdataptr;
    return xad->pos;
}

void MV_SetXAPosition(VoiceNode *voice, int32_t position)
{
    xa_data * xad = (xa_data *) voice->rawdataptr;

    if (position < XA_DATA_START || (size_t)position >= xad->length)
        position = XA_DATA_START;

    xad->pos = position;
}

/*---------------------------------------------------------------------
Function: MV_GetNextXABlock

Controls playback of XA data
---------------------------------------------------------------------*/

static playbackstatus MV_GetNextXABlock
(
 VoiceNode *voice
 )

{
    xa_data * xad = (xa_data *) voice->rawdataptr;
    XASector ssct;
    int coding;

    voice->Playing = TRUE;

	xad->t1 = xad->t2 = xad->t1_x = xad->t2_x = 0;

    do
    {
        size_t bytes = xad->length - xad->pos;

        if (sizeof(XASector) < bytes)
            bytes = sizeof(XASector);

        memcpy(&ssct, (char*) xad->ptr + xad->pos, bytes);
        xad->pos += bytes;
    }
#define SUBMODE_REAL_TIME_SECTOR (1 << 6)
#define SUBMODE_FORM             (1 << 5)
#define SUBMODE_AUDIO_DATA       (1 << 2)
    while (ssct.sectorFiller[46] != (SUBMODE_REAL_TIME_SECTOR | SUBMODE_FORM | SUBMODE_AUDIO_DATA));

    coding = ssct.sectorFiller[47];

    voice->channels = (coding & 3) + 1;
    voice->SamplingRate = (((coding >> 2) & 3) == 1) ? 18900 : 37800;

    // CODEDUP multivoc.c MV_SetVoicePitch
    voice->RateScale    = ( voice->SamplingRate * voice->PitchScale ) / MV_MixRate;
    voice->FixedPointBufferSize = ( voice->RateScale * MV_MIXBUFFERSIZE ) - voice->RateScale;
    MV_SetVoiceMixMode( voice );

    if (voice->channels == 2)
        decodeSoundSectStereo(&ssct, xad);
    else
        decodeSoundSectMono(&ssct, xad);

    voice->sound        = xad->block;
    voice->length       = (((xad->blocksize * (voice->bits/8))/2)<<voice->bits) / 4;
    voice->position     = 0;
    voice->BlockLength  = 0;

    if (xad->length == xad->pos)
    {
        if (voice->LoopSize > 0)
            xad->pos = XA_DATA_START;
        else
        {
            voice->Playing = FALSE;
            return NoMoreData;
        }
    }

    return KeepPlaying;
}


/*---------------------------------------------------------------------
Function: MV_PlayXA3D

Begin playback of sound data at specified angle and distance
from listener.
---------------------------------------------------------------------*/

int32_t MV_PlayXA3D
(
 char *ptr,
 uint32_t ptrlength,
 int32_t loophow,
 int32_t  pitchoffset,
 int32_t  angle,
 int32_t  distance,
 int32_t  priority,
 uint32_t callbackval
 )

{
   int32_t left;
   int32_t right;
   int32_t mid;
   int32_t volume;
   int32_t status;

   if ( !MV_Installed )
   {
      MV_SetErrorCode( MV_NotInstalled );
      return MV_Error;
   }

   if ( distance < 0 )
   {
      distance  = -distance;
      angle    += MV_NUMPANPOSITIONS / 2;
   }

   volume = MIX_VOLUME( distance );

   // Ensure angle is within 0 - 127
   angle &= MV_MAXPANPOSITION;

   left  = MV_PanTable[ angle ][ volume ].left;
   right = MV_PanTable[ angle ][ volume ].right;
   mid   = max( 0, 255 - distance );

   status = MV_PlayXA(ptr, ptrlength, loophow, -1, pitchoffset, mid, left, right, priority, callbackval);

   return status;
}


/*---------------------------------------------------------------------
Function: MV_PlayXA

Begin playback of sound data with the given sound levels and
priority.
---------------------------------------------------------------------*/

int32_t MV_PlayXA
(
 char *ptr,
 uint32_t ptrlength,
 int32_t   loopstart,
 int32_t   loopend,
 int32_t   pitchoffset,
 int32_t   vol,
 int32_t   left,
 int32_t   right,
 int32_t   priority,
 uint32_t callbackval
 )

{
   VoiceNode   *voice;
   xa_data * xad = 0;

   UNREFERENCED_PARAMETER(loopend);

   if ( !MV_Installed )
   {
      MV_SetErrorCode( MV_NotInstalled );
      return MV_Error;
   }

   xad = (xa_data *) calloc( 1, sizeof(xa_data) );
   if (!xad) {
      MV_SetErrorCode( MV_InvalidFile );
      return MV_Error;
   }

   xad->ptr = ptr;
   xad->pos = XA_DATA_START;
   xad->blocksize = 0;
   xad->length = ptrlength;

   xad->block = NULL;

   // Request a voice from the voice pool
   voice = MV_AllocVoice( priority );
   if ( voice == NULL )
   {

      free(xad);
      MV_SetErrorCode( MV_NoVoices );
      return MV_Error;
   }

   xad->owner = voice;

   voice->wavetype    = FMT_XA;
   voice->rawdataptr       = (void*)xad;
   voice->GetSound    = MV_GetNextXABlock;
   voice->NextBlock   = xad->block;
   voice->LoopCount   = 0;
   voice->BlockLength = 0;
   voice->PitchScale  = PITCH_GetScale( pitchoffset );
   voice->next        = NULL;
   voice->prev        = NULL;
   voice->priority    = priority;
   voice->callbackval = callbackval;

   voice->bits        = 16;

   voice->bits        = 16;

   voice->Playing     = TRUE;
   voice->Paused      = FALSE;

   voice->LoopStart   = 0;
   voice->LoopEnd     = 0;
   voice->LoopSize    = (loopstart >= 0 ? 1 : 0);

   MV_SetVoiceVolume( voice, vol, left, right );
   MV_PlayVoice( voice );

   return voice->handle;
}


void MV_ReleaseXAVoice( VoiceNode * voice )
{
    xa_data * xad = (xa_data *) voice->rawdataptr;

    if (voice->wavetype != FMT_XA) {
        return;
    }

    free(xad->block);
    free(xad);

    voice->rawdataptr = 0;
}
