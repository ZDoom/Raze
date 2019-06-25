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

#include "duke3d.h"

#include "vfs.h"


static int32_t rts_numlumps;
static void  **rts_lumpcache;
static lumpinfo_t *rts_lumpinfo;
static int32_t RTS_Started = FALSE;

char rts_lumplockbyte[11];


/*
= RTS_AddFile
=
= All files are optional, but at least one file must be found
= Files with a .rts extension are wadlink files with multiple lumps
= Other files are single lumps with the base filename for the lump name
*/
static int32_t RTS_AddFile(const char *filename)
{
    wadinfo_t  header;
    int32_t i, length, startlump;
    filelump_t *fileinfo, *fileinfoo;

    // read the entire file in
    //      FIXME: shared opens

    buildvfs_kfd handle = kopen4loadfrommod(filename, 0);

    if (handle == buildvfs_kfd_invalid)
        return -1;
    else
        initprintf("RTS file \"%s\" loaded\n", filename);

    startlump = rts_numlumps;

    // WAD file
    i = kread(handle, &header, sizeof(header));
    if (i != sizeof(header) || Bmemcmp(header.identification, "IWAD", 4))
    {
        initprintf("RTS file \"%s\" too short or doesn't have IWAD id\n", filename);
        kclose(handle);
        return -1;
    }

    header.numlumps = B_LITTLE32(header.numlumps);
    header.infotableofs = B_LITTLE32(header.infotableofs);

    length = header.numlumps*sizeof(filelump_t);
    fileinfo = fileinfoo = (filelump_t *)Xmalloc(length);

    klseek(handle, header.infotableofs, SEEK_SET);
    kread(handle, fileinfo, length);

    {
        lumpinfo_t *lump_p = (lumpinfo_t *)Xrealloc(
            rts_lumpinfo, (rts_numlumps + header.numlumps)*sizeof(lumpinfo_t));

        rts_lumpinfo = lump_p;
    }

    rts_numlumps += header.numlumps;

    for (i=startlump; i<rts_numlumps; i++, fileinfo++)
    {
        lumpinfo_t *lump = &rts_lumpinfo[i];

        lump->handle = handle;  // NOTE: cache1d-file is not closed!
        lump->position = B_LITTLE32(fileinfo->filepos);
        lump->size = B_LITTLE32(fileinfo->size);

        Bstrncpy(lump->name, fileinfo->name, 8);
    }

    Xfree(fileinfoo);

    return 0;
}


void RTS_Init(const char *filename)
{
    // open all the files, load headers, and count lumps

    rts_numlumps = 0;
    rts_lumpinfo = NULL;   // will be realloced as lumps are added

    if (RTS_AddFile(filename))
        return;

    if (rts_numlumps == 0)
        return;

    rts_lumpcache = (void **)Xcalloc(rts_numlumps, sizeof(rts_lumpcache[0]));

    RTS_Started = TRUE;
}


int32_t RTS_IsInitialized(void)
{
    return rts_numlumps > 0;
}


#define RTS_BAD_LUMP(lump) ((uint32_t)lump >= (uint32_t)rts_numlumps)

int32_t RTS_SoundLength(int32_t lump)
{
    lump++;

    return RTS_BAD_LUMP(lump) ? 0 : rts_lumpinfo[lump].size;
}


/* Loads the lump into the given buffer, which must be >= RTS_SoundLength() */
static void RTS_ReadLump(int32_t lump, void *dest)
{
    lumpinfo_t *l = &rts_lumpinfo[lump];

    klseek(l->handle, l->position, SEEK_SET);
    kread(l->handle, dest, l->size);
}


void *RTS_GetSound(int32_t lump)
{
    lump++;

    if (RTS_BAD_LUMP(lump))
        return NULL;

    if (rts_lumpcache[lump] == NULL)
    {
        rts_lumplockbyte[lump] = 200;
        cacheAllocateBlock((intptr_t *)&rts_lumpcache[lump], RTS_SoundLength(lump-1), &rts_lumplockbyte[lump]);  // JBF 20030910: char * => int32_t *
        RTS_ReadLump(lump, rts_lumpcache[lump]);
    }
    else
    {
        if (rts_lumplockbyte[lump] < 200)
            rts_lumplockbyte[lump] = 200;
        else
            rts_lumplockbyte[lump]++;
    }

    return rts_lumpcache[lump];
}
