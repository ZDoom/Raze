//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2004, 2007 - EDuke32 developers

This file is part of EDuke32

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include "duke3d.h"

//=============
// STATICS
//=============

int32 numlumps;
static void  **lumpcache;
static lumpinfo_t *lumpinfo;              // location of each lump on disk
static boolean RTS_Started = false;

char lumplockbyte[11];

/*
============================================================================

                                                LUMP BASED ROUTINES

============================================================================
*/

/*
====================
=
= RTS_AddFile
=
= All files are optional, but at least one file must be found
= Files with a .rts extension are wadlink files with multiple lumps
= Other files are single lumps with the base filename for the lump name
=
====================
*/

int32 RTS_AddFile(const char *filename)
{
    wadinfo_t  header;
    lumpinfo_t *lump_p;
    uint32     i;
    int32      handle, length;
    int32      startlump;
    filelump_t *fileinfo, *fileinfoo;

    //
    // read the entire file in
    //      FIXME: shared opens

    handle = kopen4loadfrommod((char *)filename, 0);
    if (handle < 0)
    {
        initprintf("RTS file %s was not found\n",filename);
        return -1;
    }

    startlump = numlumps;

    // WAD file
//    initprintf("    Adding %s.\n",filename);
    kread(handle, &header, sizeof(header));
    if (strncmp(header.identification,"IWAD",4))
    {
        initprintf("RTS file %s doesn't have IWAD id\n",filename);
        kclose(handle);
        return -1;
    }
    header.numlumps = IntelLong(header.numlumps);
    header.infotableofs = IntelLong(header.infotableofs);
    length = header.numlumps*sizeof(filelump_t);
    fileinfo = fileinfoo = (filelump_t*)malloc(length);
    if (!fileinfo)
    {
        initprintf("RTS file could not allocate header info\n");
        kclose(handle);
        return -1;
    }
    klseek(handle, header.infotableofs, SEEK_SET);
    kread(handle, fileinfo, length);

    //
    // Fill in lumpinfo
    //
    lump_p = realloc(lumpinfo, (numlumps + header.numlumps)*sizeof(lumpinfo_t));
    if (!lump_p)
    {
        kclose(handle);
        return -1;
    }
    lumpinfo = lump_p;

    numlumps += header.numlumps;

    lump_p = &lumpinfo[startlump];

    for (i=startlump ; i<(uint32)numlumps ; i++,lump_p++, fileinfo++)
    {
        lump_p->handle = handle;
        lump_p->position = IntelLong(fileinfo->filepos);
        lump_p->size = IntelLong(fileinfo->size);
        strncpy(lump_p->name, fileinfo->name, 8);
    }

    free(fileinfoo);

    return 0;
}

/*
====================
=
= RTS_Init
=
= Files with a .rts extension are idlink files with multiple lumps
=
====================
*/

void RTS_Init(const char *filename)
{
    int32 length;
    //
    // open all the files, load headers, and count lumps
    //
    numlumps = 0;
    lumpinfo = NULL;   // will be realloced as lumps are added

//    initprintf("RTS Manager Started.\n");
    if (RTS_AddFile(filename)) return;

    if (!numlumps) return;

    //
    // set up caching
    //
    length = (numlumps) * sizeof(*lumpcache);
    lumpcache = SafeMalloc(length);
    memset(lumpcache,0,length);
    RTS_Started = true;
}


/*
====================
=
= RTS_NumSounds
=
====================
*/

int32 RTS_NumSounds(void)
{
    return numlumps-1;
}

/*
====================
=
= RTS_SoundLength
=
= Returns the buffer size needed to load the given lump
=
====================
*/

int32 RTS_SoundLength(int32 lump)
{
    lump++;
    if (lump >= numlumps)
        Error("RTS_SoundLength: %i >= numlumps",lump);
    return lumpinfo[lump].size;
}

/*
====================
=
= RTS_GetSoundName
=
====================
*/

const char * RTS_GetSoundName(int32 i)
{
    i++;
    if (i>=numlumps)
        Error("RTS_GetSoundName: %i >= numlumps",i);
    return &(lumpinfo[i].name[0]);
}

/*
====================
=
= RTS_ReadLump
=
= Loads the lump into the given buffer, which must be >= RTS_SoundLength()
=
====================
*/
void RTS_ReadLump(int32 lump, void *dest)
{
    lumpinfo_t *l;

    if (lump >= numlumps)
        Error("RTS_ReadLump: %i >= numlumps",lump);
    if (lump < 0)
        Error("RTS_ReadLump: %i < 0",lump);
    l = lumpinfo+lump;
    klseek(l->handle, l->position, SEEK_SET);
    kread(l->handle,dest,l->size);
}

/*
====================
=
= RTS_GetSound
=
====================
*/
void *RTS_GetSound(int32 lump)
{
    lump++;
    if ((uint32)lump >= (uint32)numlumps)
        Error("RTS_GetSound: %i >= %i\n",lump,numlumps);

    if (lumpcache[lump] == NULL)
    {
        lumplockbyte[lump] = 200;
        allocache((intptr_t *)&lumpcache[lump],(int)RTS_SoundLength(lump-1),&lumplockbyte[lump]);	// JBF 20030910: char * => int *
        RTS_ReadLump(lump, lumpcache[lump]);
    }
    else
    {
        if (lumplockbyte[lump] < 200)
            lumplockbyte[lump] = 200;
        else
            lumplockbyte[lump]++;
    }
    return lumpcache[lump];
}

