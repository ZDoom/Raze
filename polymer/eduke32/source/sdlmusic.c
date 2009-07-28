/*
Copyright (C) 2003-2004 Ryan C. Gordon. and James Bentler

This program is free software; you can redistribute it and/or
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

Originally written by Ryan C. Gordon. (icculus@clutteredmind.org)
Adapted to work with JonoF's port by James Bentler (bentler@cs.umn.edu)

*/
/*
 * A reimplementation of Jim Dose's FX_MAN routines, using  SDL_mixer 1.2.
 *   Whee. FX_MAN is also known as the "Apogee Sound System", or "ASS" for
 *   short. How strangely appropriate that seems.
 */

#include <stdio.h>
#include <errno.h>

#include "duke3d.h"
#include "cache1d.h"

#define _NEED_SDLMIXER	1
#include "sdl_inc.h"
#include "music.h"

int32_t MUSIC_ErrorCode = MUSIC_Ok;

static char warningMessage[80];
static char errorMessage[80];
static int32_t external_midi = 0;

static int32_t music_initialized = 0;
static int32_t music_context = 0;
static int32_t music_loopflag = MUSIC_PlayOnce;
static Mix_Music *music_musicchunk = NULL;

static void setErrorMessage(const char *msg)
{
    Bstrncpy(errorMessage, msg, sizeof(errorMessage));
    // strncpy() doesn't add the null char if there isn't room...
    errorMessage[sizeof(errorMessage) - 1] = '\0';
} // setErrorMessage

// The music functions...

char *MUSIC_ErrorString(int32_t ErrorNumber)
{
    switch (ErrorNumber)
    {
    case MUSIC_Warning:
        return(warningMessage);

    case MUSIC_Error:
        return(errorMessage);

    case MUSIC_Ok:
        return("OK; no error.");

    case MUSIC_ASSVersion:
        return("Incorrect sound library version.");

    case MUSIC_SoundCardError:
        return("General sound card error.");

    case MUSIC_InvalidCard:
        return("Invalid sound card.");

    case MUSIC_MidiError:
        return("MIDI error.");

    case MUSIC_MPU401Error:
        return("MPU401 error.");

    case MUSIC_TaskManError:
        return("Task Manager error.");

        //case MUSIC_FMNotDetected:
        //    return("FM not detected error.");

    case MUSIC_DPMI_Error:
        return("DPMI error.");

    default:
        return("Unknown error.");
    } // switch

    return(NULL);
} // MUSIC_ErrorString

int32_t MUSIC_Init(int32_t SoundCard, int32_t Address)
{
    // Use an external MIDI player if the user has specified to do so
    char *command = getenv("EDUKE32_MUSIC_CMD");
    const SDL_version *linked = Mix_Linked_Version();

    UNREFERENCED_PARAMETER(Address);
    if (SDL_VERSIONNUM(linked->major,linked->minor,linked->patch) < MIX_REQUIREDVERSION)
    {
        // reject running with SDL_Mixer versions older than what is stated in sdl_inc.h
        initprintf("You need at least v%d.%d.%d of SDL_mixer for music\n",SDL_MIXER_MIN_X,SDL_MIXER_MIN_Y,SDL_MIXER_MIN_Z);
        return(MUSIC_Error);
    }

    external_midi = (command != NULL && command[0] != 0);

    if (external_midi)
        Mix_SetMusicCMD(command);
    else
    {
        char *s[] = { "/etc/timidity.cfg", "/etc/timidity/timidity.cfg", "/etc/timidity/freepats.cfg" };
        FILE *fp;
        uint32_t i;

        for (i = 0; i < sizeof(s)/sizeof(s[0]); i++)
        {
            fp = fopen(s[i], "r");
            if (fp == NULL)
            {
                if (i == sizeof(s)/sizeof(s[0]))
                {
                    initprintf("Error opening %s, %s or %s\n",s[0],s[1],s[2]);
                    return(MUSIC_Error);
                }
                continue;
            }
            else break;
        }
        Bfclose(fp);
    }

    if (music_initialized)
    {
        setErrorMessage("Music system is already initialized.");
        return(MUSIC_Error);
    } // if

    SoundCard = 1;

    music_initialized = 1;
    return(MUSIC_Ok);
} // MUSIC_Init


int32_t MUSIC_Shutdown(void)
{
    // TODO - make sure this is being called from the menu -- SA
    if (external_midi)
        Mix_SetMusicCMD(NULL);

    MUSIC_StopSong();
    music_context = 0;
    music_initialized = 0;
    music_loopflag = MUSIC_PlayOnce;

    return(MUSIC_Ok);
} // MUSIC_Shutdown


void MUSIC_SetMaxFMMidiChannel(int32_t channel)
{
    UNREFERENCED_PARAMETER(channel);
} // MUSIC_SetMaxFMMidiChannel


void MUSIC_SetVolume(int32_t volume)
{
    volume = max(0, volume);
    volume = min(volume, 255);

    Mix_VolumeMusic(volume >> 1);  // convert 0-255 to 0-128.
} // MUSIC_SetVolume


void MUSIC_SetMidiChannelVolume(int32_t channel, int32_t volume)
{
    UNREFERENCED_PARAMETER(channel);
    UNREFERENCED_PARAMETER(volume);
} // MUSIC_SetMidiChannelVolume


void MUSIC_ResetMidiChannelVolumes(void)
{
} // MUSIC_ResetMidiChannelVolumes


int32_t MUSIC_GetVolume(void)
{
    return(Mix_VolumeMusic(-1) << 1);  // convert 0-128 to 0-255.
} // MUSIC_GetVolume


void MUSIC_SetLoopFlag(int32_t loopflag)
{
    music_loopflag = loopflag;
} // MUSIC_SetLoopFlag


int32_t MUSIC_SongPlaying(void)
{
    return((Mix_PlayingMusic()) ? TRUE : FALSE);
} // MUSIC_SongPlaying


void MUSIC_Continue(void)
{
    if (Mix_PausedMusic())
        Mix_ResumeMusic();
} // MUSIC_Continue


void MUSIC_Pause(void)
{
    Mix_PauseMusic();
} // MUSIC_Pause


int32_t MUSIC_StopSong(void)
{
    //if (!fx_initialized)
    if (!Mix_QuerySpec(NULL, NULL, NULL))
    {
        setErrorMessage("Need FX system initialized, too. Sorry.");
        return(MUSIC_Error);
    } // if

    if ((Mix_PlayingMusic()) || (Mix_PausedMusic()))
        Mix_HaltMusic();

    if (music_musicchunk)
        Mix_FreeMusic(music_musicchunk);

    music_musicchunk = NULL;

    return(MUSIC_Ok);
} // MUSIC_StopSong

// Duke3D-specific.  --ryan.
// void MUSIC_PlayMusic(char *_filename)
int32_t MUSIC_PlaySong(char *song, int32_t loopflag)
{
    MUSIC_StopSong();
    music_musicchunk = Mix_LoadMUS_RW(SDL_RWFromMem((char *) song, g_musicSize));

    if (music_musicchunk != NULL)
    {
        Mix_PlayMusic(music_musicchunk, (loopflag == MUSIC_LoopSong)?-1:0);
    }
    return MUSIC_Ok;
}


void MUSIC_SetContext(int32_t context)
{
    music_context = context;
} // MUSIC_SetContext


int32_t MUSIC_GetContext(void)
{
    return(music_context);
} // MUSIC_GetContext


void MUSIC_SetSongTick(uint32_t PositionInTicks)
{
    UNREFERENCED_PARAMETER(PositionInTicks);
} // MUSIC_SetSongTick


void MUSIC_SetSongTime(uint32_t milliseconds)
{
    UNREFERENCED_PARAMETER(milliseconds);
}// MUSIC_SetSongTime


void MUSIC_SetSongPosition(int32_t measure, int32_t beat, int32_t tick)
{
    UNREFERENCED_PARAMETER(measure);
    UNREFERENCED_PARAMETER(beat);
    UNREFERENCED_PARAMETER(tick);
} // MUSIC_SetSongPosition


void MUSIC_GetSongPosition(songposition *pos)
{
    UNREFERENCED_PARAMETER(pos);
} // MUSIC_GetSongPosition


void MUSIC_GetSongLength(songposition *pos)
{
    UNREFERENCED_PARAMETER(pos);
} // MUSIC_GetSongLength


int32_t MUSIC_FadeVolume(int32_t tovolume, int32_t milliseconds)
{
    UNREFERENCED_PARAMETER(tovolume);
    Mix_FadeOutMusic(milliseconds);
    return(MUSIC_Ok);
} // MUSIC_FadeVolume


int32_t MUSIC_FadeActive(void)
{
    return((Mix_FadingMusic() == MIX_FADING_OUT) ? TRUE : FALSE);
} // MUSIC_FadeActive


void MUSIC_StopFade(void)
{
} // MUSIC_StopFade


void MUSIC_RerouteMidiChannel(int32_t channel, int32_t (*function)(int32_t event, int32_t c1, int32_t c2))
{
    UNREFERENCED_PARAMETER(channel);
    UNREFERENCED_PARAMETER(function);
} // MUSIC_RerouteMidiChannel


void MUSIC_RegisterTimbreBank(char *timbres)
{
    UNREFERENCED_PARAMETER(timbres);
} // MUSIC_RegisterTimbreBank


void MUSIC_Update(void)
{}
