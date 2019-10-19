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

// This object is shared by all Build games with MIDI playback!

#include "cache1d.h"
#include "compat.h"
#include "duke3d.h"
#include "music.h"
#include "vfs.h"
#include "winbits.h"

// fork/exec based external music player
#ifndef _WIN32
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define INVALID_HANDLE_VALUE -1
typedef pid_t proc_t;
#else
typedef HANDLE proc_t;
#endif

static char ** g_musicPlayerArgv;
static int32_t g_musicPlayerEnabled;
static proc_t  g_musicPlayerHandle = INVALID_HANDLE_VALUE;
static int32_t g_musicPlayerReady;
static int8_t  g_musicPlayerRestart;
static char *  g_musicPlayerCommandLine;

static char g_musicFileName[BMAX_PATH];
static int g_musicFileNameArgvPos;

char const *errorMessage;

int32_t MUSIC_Init(int32_t SoundCard, int32_t Address)
{
    // Use an external music player
    g_musicPlayerCommandLine = getenv("EDUKE32_MUSIC_CMD");

    UNREFERENCED_PARAMETER(SoundCard);
    UNREFERENCED_PARAMETER(Address);

    if (g_musicPlayerReady)
    {
        errorMessage = "MUSIC_Init: external player already initialized!";
        return MUSIC_Error;
    } // if

    g_musicPlayerEnabled = (g_musicPlayerCommandLine && g_musicPlayerCommandLine[0]);

    if (!g_musicPlayerEnabled)
    {
        errorMessage = "MUSIC_Init: no external player configured!";
        return MUSIC_Error;
    }

    initprintf("Using external music player: \"%s\"\n", g_musicPlayerCommandLine);

#ifndef _WIN32
    int32_t ws=1, numargs=0, pagesize=Bgetpagesize();
    char *c, *cmd;
    size_t sz;

    if (pagesize == -1)
    {
        errorMessage = "MUSIC_Init: unable to determine system page size";
        return MUSIC_Error;
    }
    for (c=g_musicPlayerCommandLine; *c; c++)
    {
        if (isspace(*c))
            ws = 1;
        else if (ws)
        {
            ws = 0;
            numargs++;
        }
    }

    if (numargs == 0)
    {
        errorMessage = "MUSIC_Init: not enough arguments for external player";
        return MUSIC_Error;
    }

    sz = (numargs+2)*sizeof(char *) + (c-g_musicPlayerCommandLine+1);
    sz = ((sz+pagesize-1)/pagesize)*pagesize;
    g_musicPlayerArgv = (char **)Xaligned_alloc(pagesize, sz);
    cmd = (char *)g_musicPlayerArgv + (numargs+2)*sizeof(intptr_t);
    Bmemcpy(cmd, g_musicPlayerCommandLine, c-g_musicPlayerCommandLine+1);

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
            g_musicPlayerArgv[numargs++] = c;
        }
    }
    g_musicFileNameArgvPos = numargs;
    g_musicPlayerArgv[numargs] = g_musicFileName;
    g_musicPlayerArgv[numargs+1] = NULL;

#if 0
    if (mprotect(g_musicPlayerArgv, sz, PROT_READ)==-1)  // make argv and command string read-only
    {
        initprintf("MUSIC_Init: mprotect(): %s\n");
        errorMessage = "MUSIC_Init: mprotect() failure";
        return MUSIC_Error;
    }
#endif
#endif

    g_musicPlayerReady = 1;
    return MUSIC_Ok;
}


int32_t MUSIC_Shutdown(void)
{
    MUSIC_StopSong();
    g_musicPlayerReady = 0;

    return MUSIC_Ok;
} // MUSIC_Shutdown


void MUSIC_SetMaxFMMidiChannel(int32_t channel)
{
    UNREFERENCED_PARAMETER(channel);
} // MUSIC_SetMaxFMMidiChannel

int MUSIC_Volume;

void MUSIC_SetVolume(int32_t volume)
{
    volume = max(0, volume);
    volume = min(volume, 255);

    MUSIC_Volume = volume;
} // MUSIC_SetVolume


int32_t MUSIC_GetVolume(void)
{
    return MUSIC_Volume;
} // MUSIC_GetVolume


void MUSIC_SetLoopFlag(int32_t loopflag)
{
    UNREFERENCED_PARAMETER(loopflag);
} // MUSIC_SetLoopFlag


void MUSIC_Continue(void)
{
} // MUSIC_Continue


void MUSIC_Pause(void)
{
} // MUSIC_Pause

int32_t MUSIC_StopSong(void)
{
    if (!g_musicPlayerEnabled)
        return MUSIC_Ok;

    if (g_musicPlayerHandle != INVALID_HANDLE_VALUE)
    {
        g_musicPlayerRestart = 0;  // make SIGCHLD handler a no-op

#ifndef _WIN32
        struct timespec ts;

        ts.tv_sec = 0;
        ts.tv_nsec = 5000000;  // sleep 5ms at most

        kill(g_musicPlayerHandle, SIGTERM);
        nanosleep(&ts, NULL);

        if (int ret = waitpid(g_musicPlayerHandle, NULL, WNOHANG|WUNTRACED) != g_musicPlayerHandle)
        {
            if (ret==-1)
                initprintf("%s: waitpid: %s\n", __func__, strerror(errno));
            else
            {
                // we tried to be nice, but no...
                kill(g_musicPlayerHandle, SIGKILL);

                initprintf("%s: SIGTERM timed out--trying SIGKILL\n", __func__);

                if (waitpid(g_musicPlayerHandle, NULL, WUNTRACED)==-1)
                    initprintf("%s: waitpid: %s\n", __func__, strerror(errno));
            }
        }
#else
        TerminateProcess(g_musicPlayerHandle, 0);
#endif
        g_musicPlayerHandle = INVALID_HANDLE_VALUE;
    }

    return MUSIC_Ok;
} // MUSIC_StopSong

static int32_t MUSIC_PlayExternal()
{
#ifdef _WIN32
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si,sizeof(si));
    ZeroMemory(&pi,sizeof(pi));
    si.cb = sizeof(si);

    if (!CreateProcess(NULL,g_musicPlayerCommandLine,NULL,NULL,0,0,NULL,NULL,&si,&pi))
    {
        initprintf("%s: CreateProcess: %s\n", __func__, windowsGetErrorMessage(GetLastError()));
        return MUSIC_Error;
    }
    else
        g_musicPlayerHandle = pi.hProcess;
#else
    proc_t pid = vfork();

    if (pid==-1)  // error
    {
        initprintf("%s: vfork: %s\n", __func__, strerror(errno));
        return MUSIC_Error;
    }
    else if (pid==0)  // child
    {
        // exec without PATH lookup
        if (execv(g_musicPlayerArgv[0], g_musicPlayerArgv) < 0)
        {
            initprintf("%s: execv: %s\n", __func__, strerror(errno));
            _exit(EXIT_FAILURE);
        }
    }
    else  // parent
    {
        g_musicPlayerHandle = pid;
    }
#endif
    return MUSIC_Ok;
}

#ifndef _WIN32
static void sigchld_handler(int signo)
{
    if (g_musicPlayerHandle <= 0 || !g_musicPlayerRestart || signo != SIGCHLD)
        return;

    int status;

    if (waitpid(g_musicPlayerHandle, &status, WUNTRACED)==-1)
        initprintf("%s: waitpid: %s\n", __func__, strerror(errno));

    if (WIFEXITED(status) && WEXITSTATUS(status)==0)
    {
        // loop ...
        MUSIC_PlayExternal();
    }
}
#endif

int32_t MUSIC_PlaySong(char *song, int32_t songsize, int32_t loopflag, const char *fn /*= nullptr*/)
{
    if (!g_musicPlayerEnabled)
    {
        errorMessage = "MUSIC_Init: no external player configured!";
        return MUSIC_Error;
    }

#ifndef _WIN32
    static int32_t sigchld_handler_set;

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

    auto ext = Xstrdup(fn);
    auto const c = Bsnprintf(g_musicFileName, sizeof(g_musicFileName), "%s/external%s", Bgethomedir(), strtok(ext,"."));
    g_musicFileName[c] = '\0';
    Xfree(ext);

    if (auto fp = buildvfs_fopen_write(g_musicFileName))
    {
        buildvfs_fwrite(song, 1, songsize, fp);
        buildvfs_fclose(fp);

        g_musicPlayerRestart = loopflag;
        g_musicPlayerArgv[g_musicFileNameArgvPos] = g_musicFileName;

        if (int retval = MUSIC_PlayExternal() != MUSIC_Ok)
            return retval;
    }
    else
    {
        initprintf("%s: fopen: %s\n", __func__, strerror(errno));
        return MUSIC_Error;
    }

    return MUSIC_Ok;
}


void MUSIC_Update(void)
{
#ifdef _WIN32
    if (g_musicPlayerHandle == INVALID_HANDLE_VALUE || !g_musicPlayerRestart)
        return;

    DWORD exitCode = -1;

    GetExitCodeProcess(g_musicPlayerHandle, &exitCode);

    if (exitCode != STILL_ACTIVE)
        MUSIC_PlayExternal();
#endif
}
