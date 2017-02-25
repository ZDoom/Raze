//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
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

#include "compat.h"
#include "drivers.h"
#include "multivoc.h"
#include "fx_man.h"

int32_t FX_ErrorCode = FX_Ok;
int32_t FX_Installed = FALSE;

const char *FX_ErrorString(int32_t ErrorNumber)
{
    const char *ErrorString;

    switch (ErrorNumber)
    {
        case FX_Warning:
        case FX_Error:          ErrorString = FX_ErrorString(FX_ErrorCode); break;
        case FX_Ok:             ErrorString = "Fx ok."; break;
        case FX_MultiVocError:  ErrorString = MV_ErrorString(MV_Error); break;
        default:                ErrorString = "Unknown Fx error code."; break;
    }

    return ErrorString;
}

int32_t FX_Init(int32_t numvoices, int32_t numchannels, unsigned mixrate, void *initdata)
{
    if (FX_Installed)
        FX_Shutdown();

#if defined MIXERTYPEWIN
    int SoundCard = ASS_DirectSound;
#elif defined MIXERTYPESDL
    int SoundCard = ASS_SDL;
#else
#warning No sound driver selected!
    int SoundCard = ASS_NoSound;
#endif

    if (SoundDriver_IsSupported(SoundCard) == 0)
    {
        // unsupported cards fall back to no sound
        SoundCard = ASS_NoSound;
    }

    int status = FX_Ok;

    if (MV_Init(SoundCard, mixrate, numvoices, numchannels, initdata) != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Error;
    }

    if (status == FX_Ok)
        FX_Installed = TRUE;

    return status;
}

int32_t FX_Shutdown(void)
{
    if (!FX_Installed)
        return FX_Ok;

    int status = MV_Shutdown();

    if (status != MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        status = FX_Error;
    }

    FX_Installed = FALSE;

    return status;
}

static wavefmt_t FX_DetectFormat(char const * const ptr, uint32_t length)
{
    if (length < 12)
        return FMT_UNKNOWN;

    wavefmt_t fmt = FMT_UNKNOWN;

    switch (B_LITTLE32(*(int32_t const *)ptr))
    {
        case 'C' + ('r' << 8) + ('e' << 16) + ('a' << 24):  // Crea
            fmt = FMT_VOC;
            break;
        case 'O' + ('g' << 8) + ('g' << 16) + ('S' << 24):  // OggS
            fmt = FMT_VORBIS;
            break;
        case 'R' + ('I' << 8) + ('F' << 16) + ('F' << 24):  // RIFF
            switch (B_LITTLE32(*(int32_t const *)(ptr + 8)))
            {
                case 'C' + ('D' << 8) + ('X' << 16) + ('A' << 24):  // CDXA
                    fmt = FMT_XA;
                    break;
                default: fmt = FMT_WAV; break;
            }
            break;
        case 'f' + ('L' << 8) + ('a' << 16) + ('C' << 24):  // fLaC
            fmt = FMT_FLAC;
            break;
        default:
            switch (B_LITTLE32(*(int32_t const *)(ptr + 8)))
            {
                case 'W' + ('A' << 8) + ('V' << 16) + ('E' << 24):  // WAVE
                    fmt = FMT_WAV;
                    break;
            }
            if (MV_IdentifyXMP(ptr, length))
            {
                fmt = FMT_XMP;
                break;
            }
            break;
    }

    return fmt;
}

int32_t FX_Play(char *ptr, uint32_t length, int32_t loopstart, int32_t loopend, int32_t pitchoffset,
                          int32_t vol, int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{
    static int32_t(*const func[])(char *, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, uint32_t) =
    { NULL, NULL, MV_PlayVOC, MV_PlayWAV, MV_PlayVorbis, MV_PlayFLAC, MV_PlayXA, MV_PlayXMP };

    EDUKE32_STATIC_ASSERT(FMT_MAX == ARRAY_SIZE(func));

    wavefmt_t const fmt = FX_DetectFormat(ptr, length);

    int handle =
    (func[fmt]) ? func[fmt](ptr, length, loopstart, loopend, pitchoffset, vol, left, right, priority, callbackval) : -1;

    if (handle <= MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return handle;
}

int32_t FX_Play3D(char *ptr, uint32_t length, int32_t loophow, int32_t pitchoffset, int32_t angle, int32_t distance,
                      int32_t priority, uint32_t callbackval)
{
    static int32_t (*const func[])(char *, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t, uint32_t) =
    { NULL, NULL, MV_PlayVOC3D, MV_PlayWAV3D, MV_PlayVorbis3D, MV_PlayFLAC3D, MV_PlayXA3D, MV_PlayXMP3D };

    EDUKE32_STATIC_ASSERT(FMT_MAX == ARRAY_SIZE(func));

    wavefmt_t const fmt = FX_DetectFormat(ptr, length);

    int handle =
    (func[fmt]) ? func[fmt](ptr, length, loophow, pitchoffset, angle, distance, priority, callbackval) : -1;

    if (handle <= MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return handle;
}

int32_t FX_SetPrintf(void (*function)(const char *, ...))
{
    MV_SetPrintf(function);

    return FX_Ok;
}
