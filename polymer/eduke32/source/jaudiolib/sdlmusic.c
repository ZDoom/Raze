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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>

#include "duke3d.h"
#include "cache1d.h"

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#if (defined __WATCOMC__)
// This is probably out of date.  --ryan.
#include "dukesnd_watcom.h"
#endif

#if (!defined __WATCOMC__)
#define cdecl
#endif

#include <SDL.h>
#include <SDL_mixer.h>
#include "music.h"

#ifdef USE_OPENAL
#include "openal.h"
#endif

#define __FX_TRUE  (1 == 1)
#define __FX_FALSE (!__FX_TRUE)

#define DUKESND_DEBUG       "DUKESND_DEBUG"

#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif

int MUSIC_ErrorCode = MUSIC_Ok;

static char warningMessage[80];
static char errorMessage[80];
static FILE *debug_file = NULL;
static int initialized_debugging = 0;
static int external_midi = 0;

static char *midifn = NULL;

static char ApogeePath[256] = "/tmp/";

#define PATH_SEP_CHAR '/'
#define PATH_SEP_STR  "/"
#define ROOTDIR       "/"
#define CURDIR        "./"

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

void FixFilePath(char *filename)
{
    char *ptr;
    char *lastsep = filename;

    if ((!filename) || (*filename == '\0'))
        return;

    if (access(filename, F_OK) == 0)  /* File exists; we're good to go. */
        return;

    for (ptr = filename; 1; ptr++)
    {
        if (*ptr == '\\')
            *ptr = PATH_SEP_CHAR;

        if ((*ptr == PATH_SEP_CHAR) || (*ptr == '\0'))
        {
            char pch = *ptr;
            struct dirent *dent = NULL;
            DIR *dir;

            if ((pch == PATH_SEP_CHAR) && (*(ptr + 1) == '\0'))
                return; /* eos is pathsep; we're done. */

            if (lastsep == ptr)
                continue;  /* absolute path; skip to next one. */

            *ptr = '\0';
            if (lastsep == filename)
            {
                dir = opendir((*lastsep == PATH_SEP_CHAR) ? ROOTDIR : CURDIR);

                if (*lastsep == PATH_SEP_CHAR)
                {
                    lastsep++;
                }
            }
            else
            {
                *lastsep = '\0';
                dir = opendir(filename);
                *lastsep = PATH_SEP_CHAR;
                lastsep++;
            }

            if (dir == NULL)
            {
                *ptr = PATH_SEP_CHAR;
                return;  /* maybe dir doesn't exist? give up. */
            }

            while ((dent = readdir(dir)) != NULL)
            {
                if (strcasecmp(dent->d_name, lastsep) == 0)
                {
                    /* found match; replace it. */
                    strcpy(lastsep, dent->d_name);
                    break;
                }
            }

            closedir(dir);
            *ptr = pch;
            lastsep = ptr;

            if (dent == NULL)
                return;  /* no match. oh well. */

            if (pch == '\0')  /* eos? */
                return;
        }
    }
}

int32 SafeOpenWrite(const char *_filename, int32 filetype)
{
    int handle;
    char filename[MAX_PATH];
    strncpy(filename, _filename, sizeof(filename));
    filename[sizeof(filename) - 1] = '\0';
    FixFilePath(filename);

    handle = open(filename,O_RDWR | O_BINARY | O_CREAT | O_TRUNC
                  , S_IREAD | S_IWRITE);

    if (handle == -1)
        Error("Error opening %s: %s",filename,strerror(errno));

    return handle;
}

void SafeWrite(int32 handle, void *buffer, int32 count)
{
    unsigned    iocount;

    while (count)
    {
        iocount = count > 0x8000 ? 0x8000 : count;
        if (write(handle,buffer,iocount) != (int)iocount)
            Error("File write failure writing %d bytes",count);
        buffer = (void *)((byte *)buffer + iocount);
        count -= iocount;
    }
}

void GetUnixPathFromEnvironment(char *fullname, int32 length, const char *filename)
{
    snprintf(fullname, length-1, "%s%s", ApogeePath, filename);
}

// This gets called all over the place for information and debugging messages.
//  If the user set the DUKESND_DEBUG environment variable, the messages
//  go to the file that is specified in that variable. Otherwise, they
//  are ignored for the expense of the function call. If DUKESND_DEBUG is
//  set to "-" (without the quotes), then the output goes to stdout.
static void musdebug(const char *fmt, ...)
{
    va_list ap;

    if (debug_file)
    {
        fprintf(debug_file, "DUKEMUS: ");
        va_start(ap, fmt);
        vfprintf(debug_file, fmt, ap);
        va_end(ap);
        fprintf(debug_file, "\n");
        fflush(debug_file);
    } // if
} // musdebug

static void init_debugging(void)
{
    const char *envr;

    if (initialized_debugging)
        return;

    envr = getenv(DUKESND_DEBUG);
    if (envr != NULL)
    {
        if (Bstrcmp(envr, "-") == 0)
            debug_file = stdout;
        else
            debug_file = fopen(envr, "w");

        if (debug_file == NULL)
            fprintf(stderr, "DUKESND: -WARNING- Could not open debug file!\n");
        else
            setbuf(debug_file, NULL);
    } // if

    initialized_debugging = 1;
} // init_debugging

#if 0
static void setWarningMessage(const char *msg)
{
    strncpy(warningMessage, msg, sizeof(warningMessage));
    // strncpy() doesn't add the null char if there isn't room...
    warningMessage[sizeof(warningMessage) - 1] = '\0';
    musdebug("Warning message set to [%s].", warningMessage);
} // setErrorMessage
#endif

static void setErrorMessage(const char *msg)
{
    Bstrncpy(errorMessage, msg, sizeof(errorMessage));
    // strncpy() doesn't add the null char if there isn't room...
    errorMessage[sizeof(errorMessage) - 1] = '\0';
    musdebug("Error message set to [%s].", errorMessage);
} // setErrorMessage

// The music functions...

char *MUSIC_ErrorString(int ErrorNumber)
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

    assert(0);    // shouldn't hit this point.
    return(NULL);
} // MUSIC_ErrorString


static int music_initialized = 0;
static int music_context = 0;
static int music_loopflag = MUSIC_PlayOnce;
static char *music_songdata = NULL;
static Mix_Music *music_musicchunk = NULL;

int MUSIC_Init(int SoundCard, int Address)
{
    // Use an external MIDI player if the user has specified to do so
    char *command = getenv("EDUKE32_MIDI_CMD");
    external_midi = (command != NULL && command[0] != 0);
    if(external_midi)
        Mix_SetMusicCMD(command);

    init_debugging();

    musdebug("INIT! card=>%d, address=>%d...", SoundCard, Address);

    if (music_initialized)
    {
        setErrorMessage("Music system is already initialized.");
        return(MUSIC_Error);
    } // if

    SoundCard = 1;

    music_initialized = 1;
    return(MUSIC_Ok);
} // MUSIC_Init


int MUSIC_Shutdown(void)
{
    musdebug("shutting down sound subsystem.");

    // TODO - make sure this is being called from the menu -- SA
    if(external_midi)
        Mix_SetMusicCMD(NULL);

    MUSIC_StopSong();
    music_context = 0;
    music_initialized = 0;
    music_loopflag = MUSIC_PlayOnce;

    if (midifn != NULL)
    {
        initprintf("Removing temporary file '%s'\n",midifn);
        unlink(midifn);
        Bfree(midifn);
        midifn = NULL;
    }

    return(MUSIC_Ok);
} // MUSIC_Shutdown


void MUSIC_SetMaxFMMidiChannel(int channel)
{
    musdebug("STUB ... MUSIC_SetMaxFMMidiChannel(%d).\n", channel);
} // MUSIC_SetMaxFMMidiChannel


void MUSIC_SetVolume(int volume)
{
    volume = max(0, volume);
    volume = min(volume, 255);

#ifdef USE_OPENAL
    if (!openal_disabled)
        AL_SetMusicVolume(volume);
#endif

    Mix_VolumeMusic(volume >> 1);  // convert 0-255 to 0-128.
} // MUSIC_SetVolume


void MUSIC_SetMidiChannelVolume(int channel, int volume)
{
    musdebug("STUB ... MUSIC_SetMidiChannelVolume(%d, %d).\n", channel, volume);
} // MUSIC_SetMidiChannelVolume


void MUSIC_ResetMidiChannelVolumes(void)
{
    musdebug("STUB ... MUSIC_ResetMidiChannelVolumes().\n");
} // MUSIC_ResetMidiChannelVolumes


int MUSIC_GetVolume(void)
{
    return(Mix_VolumeMusic(-1) << 1);  // convert 0-128 to 0-255.
} // MUSIC_GetVolume


void MUSIC_SetLoopFlag(int loopflag)
{
    music_loopflag = loopflag;
} // MUSIC_SetLoopFlag


int MUSIC_SongPlaying(void)
{
    return((Mix_PlayingMusic()) ? __FX_TRUE : __FX_FALSE);
} // MUSIC_SongPlaying


void MUSIC_Continue(void)
{
#ifdef USE_OPENAL
    if (!openal_disabled)
        AL_Continue();
#endif
    if (Mix_PausedMusic())
        Mix_ResumeMusic();
    else if (music_songdata)
        MUSIC_PlaySong((unsigned char *)music_songdata, MUSIC_PlayOnce);
} // MUSIC_Continue


void MUSIC_Pause(void)
{
#ifdef USE_OPENAL
    if (!openal_disabled)
        AL_Pause();
#endif
    Mix_PauseMusic();
} // MUSIC_Pause


int MUSIC_StopSong(void)
{
#ifdef USE_OPENAL
    if (!openal_disabled)
        AL_Stop();
#endif
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

    music_songdata = NULL;
    music_musicchunk = NULL;

    return(MUSIC_Ok);
} // MUSIC_StopSong


int MUSIC_PlaySong(unsigned char *song, int loopflag)
{
    //SDL_RWops *rw;
#ifdef USE_OPENAL
   if (!openal_disabled)
       AL_PlaySong((char *)song,loopflag);
   if(openal_disabled || AL_isntALmusic())
#endif
   {
    MUSIC_StopSong();

    music_songdata = (char *)song;

    // !!! FIXME: This could be a problem...SDL/SDL_mixer wants a RWops, which
    // !!! FIXME:  is an i/o abstraction. Since we already have the MIDI data
    // !!! FIXME:  in memory, we fake it with a memory-based RWops. None of
    // !!! FIXME:  this is a problem, except the RWops wants to know how big
    // !!! FIXME:  its memory block is (so it can do things like seek on an
    // !!! FIXME:  offset from the end of the block), and since we don't have
    // !!! FIXME:  this information, we have to give it SOMETHING.

    /* !!! ARGH! There's no LoadMUS_RW  ?!
    rw = SDL_RWFromMem((void *) song, (10 * 1024) * 1024);  // yikes.
    music_musicchunk = Mix_LoadMUS_RW(rw);
    Mix_PlayMusic(music_musicchunk, (loopflag == MUSIC_PlayOnce) ? 0 : -1);
    */
   }
    return(MUSIC_Ok);
} // MUSIC_PlaySong


// Duke3D-specific.  --ryan.
void PlayMusic(char *_filename)
{
    //char filename[MAX_PATH];
    //strcpy(filename, _filename);
    //FixFilePath(filename);

    char filename[BMAX_PATH];
    int handle;
    int size;
    void *song;
    int rc;

    MUSIC_StopSong();

    // Read from a groupfile, write it to disk so SDL_mixer can read it.
    //   Lame.  --ryan.
    handle = kopen4load(_filename, 0);
    if (handle == -1)
        return;

    size = kfilelength(handle);
    if (size == -1)
    {
        kclose(handle);
        return;
    } // if

    song = malloc(size);
    if (song == NULL)
    {
        kclose(handle);
        return;
    } // if

    rc = kread(handle, song, size);
    kclose(handle);
    if (rc != size)
    {
        Bfree(song);
        return;
    } // if

    // save the file somewhere, so SDL_mixer can load it
    {
        char *user = getenv("USERNAME");

        if (user) Bsprintf(tempbuf,"duke3d-%s.%d.mid",user,getpid());
        else Bsprintf(tempbuf,"duke3d.%d.mid",getpid());

        GetUnixPathFromEnvironment(filename, BMAX_PATH, tempbuf);

        handle = SafeOpenWrite(filename, filetype_binary);

        if (handle == -1)
            return;

        midifn = Bstrdup(filename);

        SafeWrite(handle, song, size);
        close(handle);
        Bfree(song);

        //music_songdata = song;

        music_musicchunk = Mix_LoadMUS(filename);
        if (music_musicchunk != NULL)
        {
            // !!! FIXME: I set the music to loop. Hope that's okay. --ryan.
            Mix_PlayMusic(music_musicchunk, -1);
        } // if
    }
}


void MUSIC_SetContext(int context)
{
    musdebug("STUB ... MUSIC_SetContext().\n");
    music_context = context;
} // MUSIC_SetContext


int MUSIC_GetContext(void)
{
    return(music_context);
} // MUSIC_GetContext


void MUSIC_SetSongTick(unsigned int PositionInTicks)
{
    musdebug("STUB ... MUSIC_SetSongTick().\n");
} // MUSIC_SetSongTick


void MUSIC_SetSongTime(unsigned int milliseconds)
{
    musdebug("STUB ... MUSIC_SetSongTime().\n");
}// MUSIC_SetSongTime


void MUSIC_SetSongPosition(int measure, int beat, int tick)
{
    musdebug("STUB ... MUSIC_SetSongPosition().\n");
} // MUSIC_SetSongPosition


void MUSIC_GetSongPosition(songposition *pos)
{
    musdebug("STUB ... MUSIC_GetSongPosition().\n");
} // MUSIC_GetSongPosition


void MUSIC_GetSongLength(songposition *pos)
{
    musdebug("STUB ... MUSIC_GetSongLength().\n");
} // MUSIC_GetSongLength


int MUSIC_FadeVolume(int tovolume, int milliseconds)
{
    Mix_FadeOutMusic(milliseconds);
    return(MUSIC_Ok);
} // MUSIC_FadeVolume


int MUSIC_FadeActive(void)
{
    return((Mix_FadingMusic() == MIX_FADING_OUT) ? __FX_TRUE : __FX_FALSE);
} // MUSIC_FadeActive


void MUSIC_StopFade(void)
{
    musdebug("STUB ... MUSIC_StopFade().\n");
} // MUSIC_StopFade


void MUSIC_RerouteMidiChannel(int channel, int cdecl(*function)(int event, int c1, int c2))
{
    musdebug("STUB ... MUSIC_RerouteMidiChannel().\n");
} // MUSIC_RerouteMidiChannel


void MUSIC_RegisterTimbreBank(unsigned char *timbres)
{
    musdebug("STUB ... MUSIC_RegisterTimbreBank().\n");
} // MUSIC_RegisterTimbreBank


void MUSIC_Update(void)
{}
