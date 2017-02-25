/*
 * file_lib.c
 * File functions to emulate MACT
 *
 * by Jonathon Fowler
 *
 * Since we weren't given the source for MACT386.LIB so I've had to do some
 * creative interpolation here.
 *
 */
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "compat.h"

#include "file_lib.h"
#include "cache1d.h"
#include "baselayer.h"

#define MaxFiles 20
static char *FileNames[MaxFiles];

int32_t SafeOpen(const char *filename, int32_t mode, int32_t sharemode)
{
    int32_t h;

    h = openfrompath(filename, mode, sharemode);
    if (h < 0)
    {
            initprintf("Error opening %s: %s", filename, strerror(errno));
            return h;
    }

    if (h < MaxFiles)
    {
        Bfree(FileNames[h]);
        FileNames[h] = (char*)Xmalloc(strlen(filename)+1);
        strcpy(FileNames[h], filename);
    }

    return h;
}

int32_t SafeOpenRead(const char *filename, int32_t filetype)
{
    switch (filetype)
    {
    case filetype_binary:
        return SafeOpen(filename, O_RDONLY|O_BINARY, BS_IREAD);
    case filetype_text:
        return SafeOpen(filename, O_RDONLY|O_TEXT, BS_IREAD);
    default:
        initprintf("SafeOpenRead: Illegal filetype specified");
        return -1;
    }
}

void SafeClose(int32_t handle)
{
    if (handle < 0) return;
    if (close(handle) < 0)
    {
        if (handle < MaxFiles)
            initprintf("Unable to close file %s", FileNames[handle]);
        else
            initprintf("Unable to close file");
        return;
    }

    if (handle < MaxFiles && FileNames[handle])
    {
        DO_FREE_AND_NULL(FileNames[handle]);
    }
}

int32_t SafeFileExists(const char *filename)
{
    if (!access(filename, F_OK)) return TRUE;
    return FALSE;
}

int32_t SafeFileLength(int32_t handle)
{
    if (handle < 0) return -1;
    return Bfilelength(handle);
}

void SafeRead(int32_t handle, void *buffer, int32_t count)
{
    int32_t b;

    b = read(handle, buffer, count);
    if (b != count)
    {
        close(handle);
        if (handle < MaxFiles)
            initprintf("File read failure %s reading %d bytes from file %s.",
                  strerror(errno), count, FileNames[handle]);
        else
            initprintf("File read failure %s reading %d bytes.",
                  strerror(errno), count);
        return;
    }
}


