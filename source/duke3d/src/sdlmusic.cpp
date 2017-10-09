//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

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

/*
 * A reimplementation of Jim Dose's FX_MAN routines, using  SDL_mixer 1.2.
 *   Whee. FX_MAN is also known as the "Apogee Sound System", or "ASS" for
 *   short. How strangely appropriate that seems.
 */

// This object is shared by all Build games with MIDI playback!

#define _NEED_SDLMIXER	1

#include "compat.h"

#include "duke3d.h"
#include "cache1d.h"

#include "sdlayer.h"
#include "music.h"

#if !defined _WIN32 && !defined(GEKKO)
//# define FORK_EXEC_MIDI 1
#endif

#if defined FORK_EXEC_MIDI  // fork/exec based external midi player
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
static char **external_midi_argv;
static pid_t external_midi_pid=-1;
static int8_t external_midi_restart=0;
#endif

#ifdef __ANDROID__ //TODO fix
static char const *external_midi_tempfn = APPBASENAME "-music.mid";
#else
static char const *external_midi_tempfn = "/tmp/" APPBASENAME "-music.mid";
#endif

static int32_t external_midi = 0;

int32_t MUSIC_ErrorCode = MUSIC_Ok;

static char warningMessage[80];
static char errorMessage[80];

static int32_t music_initialized = 0;
static int32_t music_context = 0;
static int32_t music_loopflag = MUSIC_PlayOnce;
static Mix_Music *music_musicchunk = NULL;

static void setErrorMessage(const char *msg)
{
    Bstrncpyz(errorMessage, msg, sizeof(errorMessage));
}

// The music functions...

const char *MUSIC_ErrorString(int32_t ErrorNumber)
{
    switch (ErrorNumber)
    {
    case MUSIC_Warning:
        return warningMessage;

    case MUSIC_Error:
        return errorMessage;

    case MUSIC_Ok:
        return "OK; no error.";

    case MUSIC_MidiError:
        return "MIDI error.";

    default:
        return "Unknown error.";
    } // switch

    return NULL;
} // MUSIC_ErrorString

int32_t MUSIC_Init(int32_t SoundCard, int32_t Address)
{
#ifdef __ANDROID__
	music_initialized = 1;
	return MUSIC_Ok;
#endif
    // Use an external MIDI player if the user has specified to do so
    char *command = getenv("EDUKE32_MUSIC_CMD");
    const SDL_version *linked = Mix_Linked_Version();

    UNREFERENCED_PARAMETER(SoundCard);
    UNREFERENCED_PARAMETER(Address);

    if (music_initialized)
    {
        setErrorMessage("Music system is already initialized.");
        return MUSIC_Error;
    } // if

    if (SDL_VERSIONNUM(linked->major,linked->minor,linked->patch) < MIX_REQUIREDVERSION)
    {
        // reject running with SDL_Mixer versions older than what is stated in sdl_inc.h
        initprintf("You need at least v%d.%d.%d of SDL_mixer for music\n",SDL_MIXER_MIN_X,SDL_MIXER_MIN_Y,SDL_MIXER_MIN_Z);
        return MUSIC_Error;
    }

    external_midi = (command != NULL && command[0] != 0);

    if (external_midi)
    {
#if defined FORK_EXEC_MIDI
        int32_t ws=1, numargs=0, pagesize=sysconf(_SC_PAGE_SIZE);
        char *c, *cmd;
        size_t sz;
#endif

        initprintf("Setting music command to \"%s\".\n", command);

#if !defined FORK_EXEC_MIDI
        if (Mix_SetMusicCMD(command)==-1)
        {
            perror("Mix_SetMusicCMD");
            goto fallback;
        }
#else

        if (pagesize==-1)
            goto fallback;

        for (c=command; *c; c++)
        {
            if (isspace(*c))
                ws = 1;
            else if (ws)
            {
                ws = 0;
                numargs++;
            }
        }

        if (numargs==0)
            goto fallback;

        sz = (numargs+2)*sizeof(char *) + (c-command+1);
        sz = ((sz+pagesize-1)/pagesize)*pagesize;
#if defined(__APPLE__) || defined(__ANDROID__)
        external_midi_argv = Xcalloc(1,sz+pagesize);
        external_midi_argv = (char **)((intptr_t)external_midi_argv + (pagesize-(((intptr_t)external_midi_argv)&(pagesize-1))));
#else
        if (posix_memalign((void **)&external_midi_argv, pagesize, sz))
            goto fallback;
#endif
        cmd = (char *)external_midi_argv + (numargs+2)*sizeof(char *);
        Bmemcpy(cmd, command, c-command+1);

        ws = 1;
        numargs = 0;
        for (c=cmd; *c; c++)
        {
            if (isspace(*c))
            {
                ws = 1;
                *c = 0;
            }
            else if (ws)
            {
                ws = 0;
                external_midi_argv[numargs++] = c;
            }
        }
        external_midi_argv[numargs] = external_midi_tempfn;
        external_midi_argv[numargs+1] = NULL;

        if (mprotect(external_midi_argv, sz, PROT_READ)==-1)  // make argv and command string read-only
        {
            perror("MUSIC_Init: mprotect");
            goto fallback;
        }
# if 0
        {
            int i;
            initprintf("----Music argv:\n");
            for (i=0; i<numargs+1; i++)
                initprintf("  %s\n", external_midi_argv[i]);
            initprintf("----\n");
        }
# endif
#endif
        music_initialized = 1;
        return MUSIC_Ok;

fallback:
        initprintf("Error setting music command, falling back to timidity.\n");
    }

    {
        static const char *s[] = { "/etc/timidity.cfg", "/etc/timidity/timidity.cfg", "/etc/timidity/freepats.cfg" };
        FILE *fp;
        int32_t i;

        for (i = ARRAY_SIZE(s)-1; i>=0; i--)
        {
            fp = Bfopen(s[i], "r");
            if (fp == NULL)
            {
                if (i == 0)
                {
                    initprintf("Error: couldn't open any of the following files:\n");
                    for (i = ARRAY_SIZE(s)-1; i>=0; i--)
                        initprintf("%s\n",s[i]);
                    return MUSIC_Error;
                }
                continue;
            }
            else break;
        }
        Bfclose(fp);
    }

    music_initialized = 1;
    return MUSIC_Ok;
} // MUSIC_Init


int32_t MUSIC_Shutdown(void)
{
    // TODO - make sure this is being called from the menu -- SA
#if !defined FORK_EXEC_MIDI
    if (external_midi)
        Mix_SetMusicCMD(NULL);
#endif

    MUSIC_StopSong();
    music_context = 0;
    music_initialized = 0;
    music_loopflag = MUSIC_PlayOnce;

    return MUSIC_Ok;
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


int32_t MUSIC_GetVolume(void)
{
    return (Mix_VolumeMusic(-1) << 1);  // convert 0-128 to 0-255.
} // MUSIC_GetVolume


void MUSIC_SetLoopFlag(int32_t loopflag)
{
    music_loopflag = loopflag;
} // MUSIC_SetLoopFlag


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
#if defined FORK_EXEC_MIDI
    if (external_midi)
    {
        if (external_midi_pid > 0)
        {
            int32_t ret;
            struct timespec ts;

            external_midi_restart = 0;  // make SIGCHLD handler a no-op

            ts.tv_sec = 0;
            ts.tv_nsec = 5000000;  // sleep 5ms at most

            kill(external_midi_pid, SIGTERM);
            nanosleep(&ts, NULL);
            ret = waitpid(external_midi_pid, NULL, WNOHANG|WUNTRACED);
//            printf("(%d)", ret);

            if (ret != external_midi_pid)
            {
                if (ret==-1)
                    perror("waitpid");
                else
                {
                    // we tried to be nice, but no...
                    kill(external_midi_pid, SIGKILL);
                    initprintf("%s: wait for SIGTERM timed out.\n", __func__);
                    if (waitpid(external_midi_pid, NULL, WUNTRACED)==-1)
                        perror("waitpid (2)");
                }
            }

            external_midi_pid = -1;
        }

        return MUSIC_Ok;
    }
#endif

    //if (!fx_initialized)
    if (!Mix_QuerySpec(NULL, NULL, NULL))
    {
        setErrorMessage("Need FX system initialized, too. Sorry.");
        return MUSIC_Error;
    } // if

    if ((Mix_PlayingMusic()) || (Mix_PausedMusic()))
        Mix_HaltMusic();

    if (music_musicchunk)
        Mix_FreeMusic(music_musicchunk);

    music_musicchunk = NULL;

    return MUSIC_Ok;
} // MUSIC_StopSong

#if defined FORK_EXEC_MIDI
static void playmusic()
{
    pid_t pid = vfork();

    if (pid==-1)  // error
    {
        initprintf("%s: vfork: %s\n", __func__, strerror(errno));
    }
    else if (pid==0)  // child
    {
        // exec without PATH lookup
        if (execv(external_midi_argv[0], external_midi_argv) < 0)
        {
            perror("execv");
            _exit(1);
        }
    }
    else  // parent
    {
        external_midi_pid = pid;
    }
}

static void sigchld_handler(int signo)
{
    if (signo==SIGCHLD && external_midi_restart)
    {
        int status;

        if (external_midi_pid > 0)
        {
            if (waitpid(external_midi_pid, &status, WUNTRACED)==-1)
                perror("waitpid (3)");

            if (WIFEXITED(status) && WEXITSTATUS(status)==0)
            {
                // loop ...
                playmusic();
            }
        }
    }
}
#endif

// Duke3D-specific.  --ryan.
// void MUSIC_PlayMusic(char *_filename)
int32_t MUSIC_PlaySong(char *song, int32_t loopflag)
{
//	initprintf("MUSIC_PlaySong");
    MUSIC_StopSong();

    if (external_midi)
    {
        FILE *fp;

#if defined FORK_EXEC_MIDI
        static int32_t sigchld_handler_set = 0;

        if (!sigchld_handler_set)
        {
            struct sigaction sa;
            sa.sa_handler=sigchld_handler;
            sa.sa_flags=0;
            sigemptyset(&sa.sa_mask);

            if (sigaction(SIGCHLD, &sa, NULL)==-1)
                initprintf("%s: sigaction: %s\n", __func__, strerror(errno));

            sigchld_handler_set = 1;
        }
#endif

        fp = Bfopen(external_midi_tempfn, "wb");
        if (fp)
        {
            fwrite(song, 1, g_musicSize, fp);
            Bfclose(fp);

#if defined FORK_EXEC_MIDI
            external_midi_restart = loopflag;
            playmusic();
#else
            music_musicchunk = Mix_LoadMUS(external_midi_tempfn);
            if (!music_musicchunk)
                initprintf("Mix_LoadMUS: %s\n", Mix_GetError());
#endif
        }
        else initprintf("%s: fopen: %s\n", __func__, strerror(errno));
    }
    else
        music_musicchunk = Mix_LoadMUS_RW(SDL_RWFromMem(song, g_musicSize)
#if (SDL_MAJOR_VERSION > 1)
            , SDL_FALSE
#endif
            );

    if (music_musicchunk != NULL)
        if (Mix_PlayMusic(music_musicchunk, (loopflag == MUSIC_LoopSong)?-1:0) == -1)
            initprintf("Mix_PlayMusic: %s\n", Mix_GetError());

    return MUSIC_Ok;
}


void MUSIC_Update(void)
{}
