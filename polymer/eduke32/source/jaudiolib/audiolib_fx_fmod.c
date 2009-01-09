//-------------------------------------------------------------------------
/*
Duke Nukem Copyright (C) 1996, 2003 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  FMOD AudioLib implementation by Jonathon Fowler (jonof@edgenetwk.com)
*/
//-------------------------------------------------------------------------

#include "fx_man_fmod.h"
#include "duke3d.h"

#include "fmod.h"

#define TRUE (1==1)
#define FALSE (1==0)

#define dprintOSD(...)

#ifdef WINDOWS
extern int32_t hWindow;
#endif


void(*FX_CallBackFunc)(uint32_t) = NULL;
int32_t FX_ErrorCode = FX_Ok;

#define FX_SetErrorCode( status ) \
   FX_ErrorCode = ( status );

FSOUND_SAMPLE * FX_Samples[MAXSOUNDS + 11];	// 11 remote ridicules

int32_t FX_NumVoices = 0;
static char chtoggle=0;
static char *chstates=NULL, *chstatesa, *chstatesb;
static int32_t *chcallvals=NULL;

int32_t FX_ReadVOCInfo(char *data, int32_t size, int32_t *samplerate, int32_t *channels, int32_t *samplesize, int32_t *datalen);
int32_t FX_ReadVOCData(char *data, char *buf, int32_t bufferlen, char eightbit);

int32_t FX_SimulateCallbacks(void);


/*---------------------------------------------------------------------
   Function: FX_ErrorString

   Returns a pointer to the error message associated with an error
   number.  A -1 returns a pointer the current error.
---------------------------------------------------------------------*/

char *FX_ErrorString(int32_t ErrorNumber)
{
    char *ErrorString;

    switch (ErrorNumber)
    {
    case FX_Warning :
    case FX_Error :
        ErrorString = FX_ErrorString(FX_ErrorCode);
        break;

    case FX_Ok :
        ErrorString = "Fx ok.";
        break;

    case FX_ASSVersion :
        ErrorString = "Apogee Sound System Version WinMM  "
                      "Programmed by Jim Dose, Ported by Jonathon Fowler\n"
                      "(c) Copyright 1995 James R. Dose.  All Rights Reserved.\n";
        break;

    case FX_FMODInit :
        ErrorString = "Failed initializing FMOD.";
        break;

    default :
        ErrorString = "Unknown Fx error code.";
        break;
    }

    return(ErrorString);
}


/*---------------------------------------------------------------------
   Function: FX_Init

   Selects which sound device to use.
---------------------------------------------------------------------*/

static char *OutputType(int32_t a)
{
    switch (a)
    {
    case FSOUND_OUTPUT_NOSOUND: return "no-sound";
    case FSOUND_OUTPUT_WINMM: return "WinMM";
    case FSOUND_OUTPUT_DSOUND: return "DirectSound";
    case FSOUND_OUTPUT_A3D: return "Aureal3D";
    case FSOUND_OUTPUT_OSS: return "OSS";
    case FSOUND_OUTPUT_ESD: return "ESD";
    case FSOUND_OUTPUT_ALSA: return "ALSA";
    case FSOUND_OUTPUT_ASIO: return "ASIO";
    case FSOUND_OUTPUT_XBOX: return "Xbox";
    case FSOUND_OUTPUT_PS2: return "Playstation2";
    case FSOUND_OUTPUT_MAC: return "Macintosh Sound Manager";
    default: return "unknown";
    }
}

int32_t FX_Init(int32_t SoundCard, int32_t numvoices, int32_t numchannels, int32_t samplebits, unsigned mixrate)
{
    FSOUND_Close();

    memset(FX_Samples, 0, sizeof(FX_Samples));

#ifdef WINDOWS
    if (hWindow)
    {
        //FSOUND_SetHWND(&hWindow);
    }
#endif
    if (!FSOUND_Init(mixrate, numvoices, FSOUND_INIT_GLOBALFOCUS))
    {
        FX_SetErrorCode(FX_FMODInit);
        return FX_Error;
    }

    printOSD("FX_Init(): %d voices, %d channels, %dHz samplerate\n", numvoices,numchannels,FSOUND_GetOutputRate());
    printOSD("FX_Init(): FMOD is using the %s output driver\n", OutputType(FSOUND_GetOutput()));

    chtoggle=0;
    if (chstates) free(chstates);
    chstates = (char*)malloc(numvoices*2 + sizeof(int32_t)*numvoices);
    memset(chstates,0,numvoices*2 + sizeof(int32_t)*numvoices);

    chcallvals = (int32_t*)(chstates + numvoices*2);
    chstatesa = chstates;
    chstatesb = chstates + numvoices;

    FX_NumVoices = numvoices;

    FX_SetErrorCode(FX_Ok);
    return FX_Ok;
}


/*---------------------------------------------------------------------
   Function: FX_Shutdown

   Terminates use of sound device.
---------------------------------------------------------------------*/

int32_t FX_Shutdown(void)
{
    uint32_t curalloced, maxalloced;

    if (chstates)
    {
        FSOUND_GetMemoryStats(&curalloced, &maxalloced);
        printOSD("FX_Shutdown(): allocation stats - currently %d bytes, maximum %d bytes\n",curalloced,maxalloced);
    }

    FSOUND_Close();

    if (chstates) free(chstates);
    chstates=chstatesa=chstatesb=0;

    FX_SetErrorCode(FX_Ok);
    return FX_Ok;
}


/*---------------------------------------------------------------------
   Function: FX_SetCallback

   Sets the function to call when a voice is done.
---------------------------------------------------------------------*/

int32_t FX_SetCallBack(void(*function)(uint32_t))
{
    FX_CallBackFunc = function;
    FX_SetErrorCode(FX_Ok);
    return FX_Ok;
}


/*---------------------------------------------------------------------
   Function: FX_SetVolume

   Sets the volume of the current sound device.
---------------------------------------------------------------------*/

void FX_SetVolume(int32_t volume)
{
    FSOUND_SetSFXMasterVolume(volume);
}


/*---------------------------------------------------------------------
   Function: FX_SetReverseStereo

   Set the orientation of the left and right channels.
---------------------------------------------------------------------*/

void FX_SetReverseStereo(int32_t setting)
{}


/*---------------------------------------------------------------------
   Function: FX_GetReverseStereo

   Returns the orientation of the left and right channels.
---------------------------------------------------------------------*/

int32_t FX_GetReverseStereo(void)
{
    return 0;
}


/*---------------------------------------------------------------------
   Function: FX_SetReverb

   Sets the reverb level.
---------------------------------------------------------------------*/

void FX_SetReverb(int32_t reverb)
{}


/*---------------------------------------------------------------------
   Function: FX_SetReverbDelay

   Sets the delay level of reverb to add to mix.
---------------------------------------------------------------------*/

void FX_SetReverbDelay(int32_t delay)
{}


/*---------------------------------------------------------------------
   Function: FX_VoiceAvailable

   Checks if a voice can be play at the specified priority.
---------------------------------------------------------------------*/

int32_t FX_VoiceAvailable(int32_t priority)
{
    FX_SimulateCallbacks();
    return 1;
}


/*---------------------------------------------------------------------
   Function: FX_PlayLoopedVOC

   Begin playback of sound data with the given volume and priority.
   JBF: As a hack, since Duke3D passes the sound/sample number as the
   callbackval parameter, we can use this as an index into the
   FX_Samples array to access samples if they've already been loaded.
   RemoteRidicule sounds have negative callback values, so they
   take up residence at the end of FX_Samples.
---------------------------------------------------------------------*/

int32_t FX_PlayLoopedVOC
(
    char *ptr,
    int32_t loopstart,
    int32_t loopend,
    int32_t pitchoffset,
    int32_t vol,
    int32_t left,
    int32_t right,
    int32_t priority,
    uint32_t callbackval
)
{
    return FX_PlayLoopedSound(pitchoffset, vol, callbackval);
}


/*---------------------------------------------------------------------
   Function: FX_PlayWAV

   Begin playback of sound data with the given volume and priority.
---------------------------------------------------------------------*/

int32_t FX_PlayLoopedWAV
(
    char *ptr,
    int32_t loopstart,
    int32_t loopend,
    int32_t pitchoffset,
    int32_t vol,
    int32_t left,
    int32_t right,
    int32_t priority,
    uint32_t callbackval
)
{
    return FX_PlayLoopedSound(pitchoffset, vol, callbackval);
}


/*---------------------------------------------------------------------
   Function: FX_PlayVOC3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int32_t FX_PlayVOC3D
(
    char *ptr,
    int32_t pitchoffset,
    int32_t angle,
    int32_t distance,
    int32_t priority,
    uint32_t callbackval
)
{
    return FX_PlayPositionedSound(pitchoffset, angle, distance, callbackval);
}


/*---------------------------------------------------------------------
   Function: FX_PlayWAV3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int32_t FX_PlayWAV3D
(
    char *ptr,
    int32_t pitchoffset,
    int32_t angle,
    int32_t distance,
    int32_t priority,
    uint32_t callbackval
)
{
    return FX_PlayPositionedSound(pitchoffset, angle, distance, callbackval);
}


/*---------------------------------------------------------------------
   Function: FX_Pan3D

   Set the angle and distance from the listener of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int32_t FX_Pan3D
(
    int32_t handle,
    int32_t angle,
    int32_t distance
)
{
    return FX_Ok;
}


/*---------------------------------------------------------------------
   Function: FX_StopSound

   Halts playback of a specific voice
---------------------------------------------------------------------*/

int32_t FX_StopSound(int32_t handle)
{
    FX_SimulateCallbacks();
    FSOUND_StopSound(handle);

    if (handle>=0 && handle<FX_NumVoices)
        chstatesa[handle] = chstatesb[handle] = 0;	// no callback for you!

    FX_SetErrorCode(FX_Ok);
    return FX_Ok;
}


/*---------------------------------------------------------------------
   Function: FX_StopAllSounds

   Halts playback of all sounds.
---------------------------------------------------------------------*/

int32_t FX_StopAllSounds(void)
{
    FX_SimulateCallbacks();
    FSOUND_StopSound(FSOUND_ALL);

    memset(chstates, 0, FX_NumVoices*2);		// no callbacks for any of you!

    FX_SetErrorCode(FX_Ok);
    return FX_Ok;
}


/*---------------------------------------------------------------------
   Function: FX_Play*Sound

   Internal function to load a sound file and play it, returning
   the channel number it played on.
---------------------------------------------------------------------*/
int32_t FX_PlayLoopedSound(
    int32_t pitchoffset,
    int32_t vol,
    uint32_t num
)
{
    int32_t chan;

    FX_SimulateCallbacks();

    if (!FX_Samples[num]) return -1;

    chan = FSOUND_PlaySoundEx(FSOUND_FREE, FX_Samples[num], NULL, TRUE) & 4095;
    if (chan < 0) return -1;

    // channel was already playing
    if (chstatesa[chan] && FX_CallBackFunc) FX_CallBackFunc(chcallvals[chan]);

    // set pitch
    FSOUND_SetVolume(chan, vol);
    if (FSOUND_Sample_GetMode(FX_Samples[num]) & FSOUND_STEREO)
        FSOUND_SetPan(chan, FSOUND_STEREOPAN);
    else
        FSOUND_SetPan(chan, 128);
    FSOUND_SetLoopMode(chan, FSOUND_LOOP_NORMAL);

    chcallvals[chan] = num;
    FSOUND_SetPaused(chan, FALSE);

    dprintOSD("FX_PlayLoopedSound(): Play sound %d in channel %d\n",num,chan);

    return chan;
}


int32_t FX_PlayPositionedSound(
    int32_t pitchoffset,
    int32_t angle,
    int32_t distance,
    uint32_t num
)
{
    int32_t chan;

    FX_SimulateCallbacks();

    if (!FX_Samples[num]) return -1;

    chan = FSOUND_PlaySoundEx(FSOUND_FREE, FX_Samples[num], NULL, TRUE) & 4095;
    if (chan < 0) return -1;

    // channel was already playing
    if (chstatesa[chan] && FX_CallBackFunc) FX_CallBackFunc(chcallvals[chan]);

    if (angle<0) angle = 255-angle;	// behind us

    // set pitch
    FSOUND_SetVolume(chan,255);
    FSOUND_SetPan(chan,128);
    FSOUND_SetLoopMode(chan, FSOUND_LOOP_OFF);

    chcallvals[chan] = num;
    FSOUND_SetPaused(chan, FALSE);

    dprintOSD("FX_PlayPositionedSound(): Play sound %d in channel %d\n",num,chan);

    return chan;
}


int32_t FX_LoadSample(char *ptr, int32_t size, uint32_t number, int32_t priority)
{
    FSOUND_SAMPLE *samp = NULL;
    int32_t samplerate=0, channels=0, samplesize=0;
    int32_t flags=0;
    int32_t datalen=0;
    void *ptr1,*ptr2;
    int32_t ptr1len,ptr2len;

    if (!memcmp(ptr, "Creative Voice File", 0x13))
    {
        // VOC file
        if (FX_ReadVOCInfo(ptr,size,&samplerate,&channels,&samplesize,&datalen) == 0)
        {
            flags |= (channels==2)?FSOUND_STEREO:FSOUND_MONO;
            flags |= (samplesize==16)?FSOUND_16BITS:FSOUND_8BITS;
            flags |= FSOUND_SIGNED;
            samp = FSOUND_Sample_Alloc(number, (datalen >> (channels-1)) / (samplesize>>3), flags, samplerate, -1, -1, priority);
            if (samp)
            {
                if (FSOUND_Sample_Lock(samp,0,datalen,&ptr1,&ptr2,&ptr1len,&ptr2len))
                {
                    if (FX_ReadVOCData(ptr,ptr1,datalen,(samplesize==8))) ;
                    FSOUND_Sample_Unlock(samp,ptr1,ptr2,ptr1len,ptr2len);
                }
            }
        }
    }
    else
    {
        samp = FSOUND_Sample_Load(number, ptr, FSOUND_LOADMEMORY, size);
    }

    dprintOSD("FX_LoadSample(): loaded sound %d\n",number);
    if (samp) FSOUND_Sample_SetDefaults(samp, -1, -1, -1, priority);
    FX_Samples[number] = samp;

    return (samp != NULL);
}


int32_t FX_SampleLoaded(uint32_t number)
{
    return (FX_Samples[number] != NULL);
}


#define REGRESSTC(tc) (256000000/(65536-(tc)))
#define REGRESSSR(sr) (1000000/(256-(sr)))

int32_t FX_ReadVOCInfo(char *data, int32_t size, int32_t *samplerate, int32_t *channels, int32_t *samplesize, int32_t *datalen)
{
    int16_t version,version2;
    char blocktype=0;
    int32_t  blocklen=0;
    char *ptr=data;

    if (memcmp(ptr, "Creative Voice File\x1A", 0x14)) return -1;
    ptr += 0x14;

    ptr += 2;
    version = ((int16_t*)ptr)[0];
    version2 = ((int16_t*)ptr)[1];

    if (~version + 0x1234 != version2) return -1;

    ptr += 4;

    while (1)
    {
        blocktype = *(ptr++);
        if ((ptr-data)>size) return -1;	// truncated

        if (blocktype == 0)
            break;

        blocklen = *(ptr++);
        blocklen |= *(ptr++) << 8;
        blocklen |= *(ptr++) << 16;

        switch (blocktype)
        {
        case 1: /* sound data begin block */
            if (!*samplerate)
                *samplerate = REGRESSSR(ptr[0]);
            if (ptr[1] != 0)
            {
                /* only 8-bit files please */
                return -1;
            }
            if (!*channels) *channels = 1;
            *samplesize = 8;
            *datalen += blocklen-2;
            ptr += blocklen;
            break;

        case 2: /* sound continue */
            *datalen += blocklen;
            ptr += blocklen;
            break;

#if 0
        case 3: /* silence */
            kread(fh, blockprop, 3);
            /*
            length = blockprop[0] | (blockprop[1] << 8)
            sample rate = REGRESSSR(blockprop[2]))
            */
            break;

        case 4:	/* marker */
            kread(fh, &blockprop, 2);
            /*
            id = blockprop[0] | (blockprop[1] << 8))
            */
            break;

        case 5: /* ASCII data */
            klseek(fh, blocklen, SEEK_CUR);
            /*
            asciiz string
            */
            break;

        case 6: /* repeat */
            kread(fh, blockprop, 2);
            /*
            num repetitions = (blockprop[0] | (blockprop[1] << 8)) - 1
            */
            break;

        case 7: /* end repeat */
            break;
#endif

        case 8: /* sound attribute extension block */
            *samplerate = REGRESSTC(ptr[0] | (ptr[1] << 8));
            *samplesize = 8;
            if (ptr[3] == 1)
            {
                *samplerate >>= 1;
                *channels = 2;
            }
            else
                *channels = 1;
            if (ptr[2] != 0)
            {
                /* only 8-bit files please */
                return -1;
            }
            ptr += 4;
            /* a block 1 follows */
            break;

        case 9: /* sound data format 3 */
            *samplerate = *((int32_t*)(ptr));
            *samplesize = ptr[4];
            *channels   = ptr[5];
            if ((ptr[6] | (ptr[7] << 8)) != 0 &&
                    (ptr[6] | (ptr[7] << 8)) != 4)
            {
                /* only PCM please */
                return -1;
            }
            *datalen += blocklen-12;
            ptr += blocklen;
            break;

        default:
            ptr += blocklen;
            break;
        }
    }

    return 0;
}


int32_t FX_ReadVOCData(char *data, char *buf, int32_t bufferlen, char eightbit)
{
    int16_t offset;
    char blocktype=0;
    int32_t  blocklen=0, br;

    data += 0x14 + 2 + 4;

    while (bufferlen>0)
    {
        blocktype = *(data++);

        if (blocktype == 0)
            break;

        blocklen = *(data++);
        blocklen |= *(data++) << 8;
        blocklen |= *(data++) << 16;

        switch (blocktype)
        {
        case 1: /* sound data */
            data += 2;

            br = min(blocklen-2, bufferlen);
            goto convertdata;

        case 2: /* sound continue */
            br = min(blocklen, bufferlen);
            goto convertdata;

        case 9: /* sound data format 3 */
            data += 12;

            br = min(blocklen-12, bufferlen);
            goto convertdata;

        default:
            data += blocklen;
            continue;
        }
convertdata:
        bufferlen -= br;
        if (eightbit)
        {
            // FMOD wants signed data
            for (; br>0; br--)
                *(buf++) = (uint8_t)((int16_t)(*(data++)) - 0x80);
        }
        else
        {
            memcpy(buf,data,br);
            buf += br;
            data += br;
        }
    }

    return 0;
}


int32_t FX_SimulateCallbacks(void)
{
    int32_t i;

    if (!FX_CallBackFunc || !chstates) return 0;

    chstatesa = chstates + (FX_NumVoices * chtoggle);
    chstatesb = chstates + (FX_NumVoices * (chtoggle^1));

    for (i=0;i<FX_NumVoices;i++)
    {
        chstatesa[i] = FSOUND_IsPlaying(i);
        if (chstatesa[i] == chstatesb[i]) continue;	// channel is still silent/playing
        if (chstatesa[i] > chstatesb[i]) continue;	// channel has begun playing

        // channel has ended playing
        FX_CallBackFunc(chcallvals[i]);
        dprintOSD("FX_SimulateCallbacks(): channel %d ended sound %d\n",i,chcallvals[i]);
    }

    chtoggle ^= 1;

    return 0;
}

