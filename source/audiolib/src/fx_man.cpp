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

#include "fx_man.h"

#include "compat.h"
#include "drivers.h"
#include "driver_adlib.h"
#include "midi.h"
#include "multivoc.h"
#include "osd.h"

int FX_ErrorCode = FX_Ok;
int FX_Installed;

const char *FX_ErrorString(int ErrorNumber)
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

static int osdcmd_cvar_set_audiolib(osdcmdptr_t parm)
{
    int32_t r = osdcmd_cvar_set(parm);

    if (r != OSDCMD_OK) return r;

    if (!Bstrcasecmp(parm->name, "mus_emidicard"))
    {
        MIDI_Restart();
        return r;
    }

    return r;
}

int FX_Init(int numvoices, int numchannels, unsigned mixrate, void *initdata)
{
    if (FX_Installed)
        FX_Shutdown();
    else
    {
        static int init;

        static osdcvardata_t cvars_audiolib[] = {
            { "mus_emidicard", "force a specific EMIDI instrument set", (void *)&ASS_EMIDICard, CVAR_INT | CVAR_FUNCPTR, 0, 10 },
            { "mus_adlibstereo", "enable/disable OPL3 stereo mode", (void *)&AL_Stereo, CVAR_BOOL | CVAR_FUNCPTR, 0, 10 },
        };

        if (!init++)
        {
            for (auto &i : cvars_audiolib)
                OSD_RegisterCvar(&i, (i.flags & CVAR_FUNCPTR) ? osdcmd_cvar_set_audiolib : osdcmd_cvar_set);
        }
    }
    int SoundCard = ASS_AutoDetect;

    if (SoundCard == ASS_AutoDetect) {
#if defined RENDERTYPESDL
        SoundCard = ASS_SDL;
#elif defined RENDERTYPEWIN
        SoundCard = ASS_DirectSound;
#else
        SoundCard = ASS_NoSound;
#endif
    }

    if (SoundCard < 0 || SoundCard >= ASS_NumSoundCards)
    {
        FX_SetErrorCode(FX_InvalidCard);
        return FX_Error;
    }


    if (SoundDriver_IsPCMSupported(SoundCard) == 0)
    {
        // unsupported cards fall back to no sound
        MV_Printf("Couldn't init %s, falling back to no sound...\n", SoundDriver_GetName(SoundCard));
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

int FX_Shutdown(void)
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

    switch (B_LITTLE32(*(int const *)ptr))
    {
        case 'C' + ('r' << 8) + ('e' << 16) + ('a' << 24):  // Crea
            fmt = FMT_VOC;
            break;
        case 'O' + ('g' << 8) + ('g' << 16) + ('S' << 24):  // OggS
            fmt = FMT_VORBIS;
            break;
        case 'R' + ('I' << 8) + ('F' << 16) + ('F' << 24):  // RIFF
            switch (B_LITTLE32(*(int const *)(ptr + 8)))
            {
                case 'C' + ('D' << 8) + ('X' << 16) + ('A' << 24):  // CDXA
                    fmt = FMT_XA;
                    break;
                case 'W' + ('A' << 8) + ('V' << 16) + ('E' << 24):  // WAVE
                    fmt = FMT_WAV;
                    break;
            }
            break;
        case 'f' + ('L' << 8) + ('a' << 16) + ('C' << 24):  // fLaC
            fmt = FMT_FLAC;
            break;
        default:
            if (MV_IdentifyXMP(ptr, length))
                fmt = FMT_XMP;
            break;
    }

    return fmt;
}

int FX_Play(char *ptr, uint32_t ptrlength, int loopstart, int loopend, int pitchoffset,
                          int vol, int left, int right, int priority, float volume, uint32_t callbackval)
{
    static constexpr decltype(MV_PlayVOC) *func[] =
    { nullptr, nullptr, MV_PlayVOC, MV_PlayWAV, MV_PlayVorbis, MV_PlayFLAC, MV_PlayXA, MV_PlayXMP };

    EDUKE32_STATIC_ASSERT(FMT_MAX == ARRAY_SIZE(func));

    wavefmt_t const fmt = FX_DetectFormat(ptr, ptrlength);

    int handle =
    (func[fmt]) ? func[fmt](ptr, ptrlength, loopstart, loopend, pitchoffset, vol, left, right, priority, volume, callbackval) : -1;

    if (handle <= MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return handle;
}

int FX_Play3D(char *ptr, uint32_t ptrlength, int loophow, int pitchoffset, int angle, int distance,
                      int priority, float volume, uint32_t callbackval)
{
    static constexpr decltype(MV_PlayVOC3D) *func[] =
    { nullptr, nullptr, MV_PlayVOC3D, MV_PlayWAV3D, MV_PlayVorbis3D, MV_PlayFLAC3D, MV_PlayXA3D, MV_PlayXMP3D };

    EDUKE32_STATIC_ASSERT(FMT_MAX == ARRAY_SIZE(func));

    wavefmt_t const fmt = FX_DetectFormat(ptr, ptrlength);

    int handle =
    (func[fmt]) ? func[fmt](ptr, ptrlength, loophow, pitchoffset, angle, distance, priority, volume, callbackval) : -1;

    if (handle <= MV_Ok)
    {
        FX_SetErrorCode(FX_MultiVocError);
        handle = FX_Warning;
    }

    return handle;
}

int FX_SetPrintf(void (*function)(const char *, ...))
{
    MV_SetPrintf(function);

    return FX_Ok;
}
