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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include "compat.h"
#include "baselayer.h"

#include "scriptfile.h"
#include "cache1d.h"
#include "crc32.h"

#include "duke3d.h"
#include "grpscan.h"

struct grpfile grpfiles[numgrpfiles] =
{
    { "Duke Nukem 3D",						    0xBBC9CE44, 26524524, GAMEDUKE, NULL },
    { "Duke Nukem 3D: Atomic Edition",		    0xF514A6AC, 44348015, GAMEDUKE, NULL },
    { "Duke Nukem 3D: Atomic Edition",		    0xFD3DCFF1, 44356548, GAMEDUKE, NULL },
    { "Duke Nukem 3D Shareware",		        0x983AD923, 11035779, GAMEDUKE, NULL },
    { "Duke Nukem 3D Mac Shareware",	        0xC5F71561, 10444391, GAMEDUKE, NULL },
    { "NAM",                        			0x75C1F07B, 43448927, GAMENAM,  NULL },
    { "Napalm",                        			0x3DE1589A, 44365728, GAMENAM,  NULL },
    { "WW2GI",                         			0x907B82BF, 77939508, GAMEWW2,  NULL },
};
struct grpfile *foundgrps = NULL;

#define GRPCACHEFILE "grpfiles.cache"
static struct grpcache
{
    struct grpcache *next;
    int32_t size;
    int32_t mtime;
    int32_t crcval;
    char name[BMAX_PATH];
}
*grpcache = NULL, *usedgrpcache = NULL;

static int32_t LoadGroupsCache(void)
{
    struct grpcache *fg;

    int32_t fsize, fmtime, fcrcval;
    char *fname;

    scriptfile *script;

    script = scriptfile_fromfile(GRPCACHEFILE);
    if (!script) return -1;

    while (!scriptfile_eof(script))
    {
        if (scriptfile_getstring(script, &fname)) break;	// filename
        if (scriptfile_getnumber(script, &fsize)) break;	// filesize
        if (scriptfile_getnumber(script, &fmtime)) break;	// modification time
        if (scriptfile_getnumber(script, &fcrcval)) break;	// crc checksum

        fg = Bcalloc(1, sizeof(struct grpcache));
        fg->next = grpcache;
        grpcache = fg;

        Bstrncpy(fg->name, fname, BMAX_PATH);
        fg->size = fsize;
        fg->mtime = fmtime;
        fg->crcval = fcrcval;
    }

    scriptfile_close(script);
    return 0;
}

static void FreeGroupsCache(void)
{
    struct grpcache *fg;

    while (grpcache)
    {
        fg = grpcache->next;
        Bfree(grpcache);
        grpcache = fg;
    }
}

int32_t ScanGroups(void)
{
    CACHE1D_FIND_REC *srch, *sidx;
    struct grpcache *fg, *fgg;
    struct grpfile *grp;
    char *fn;
    struct Bstat st;
#define BUFFER_SIZE (1024 * 1024 * 8)
    uint8_t *buf = Bmalloc(BUFFER_SIZE);

    if (!buf)
    {
        initprintf("Error allocating %d byte buffer to scan GRPs!\n", BUFFER_SIZE);
        return 0;
    }

    initprintf("Searching for game data...\n");

    LoadGroupsCache();

    srch = klistpath("/", "*.grp", CACHE1D_FIND_FILE);

    for (sidx = srch; sidx; sidx = sidx->next)
    {
        for (fg = grpcache; fg; fg = fg->next)
        {
            if (!Bstrcmp(fg->name, sidx->name)) break;
        }

        if (fg)
        {
            if (findfrompath(sidx->name, &fn)) continue;	// failed to resolve the filename
            if (Bstat(fn, &st))
            {
                Bfree(fn);
                continue;
            }	// failed to stat the file
            Bfree(fn);
            if (fg->size == st.st_size && fg->mtime == st.st_mtime)
            {
                grp = (struct grpfile *)Bcalloc(1, sizeof(struct grpfile));
                grp->name = Bstrdup(sidx->name);
                grp->crcval = fg->crcval;
                grp->size = fg->size;
                grp->next = foundgrps;
                foundgrps = grp;

                fgg = (struct grpcache *)Bcalloc(1, sizeof(struct grpcache));
                strcpy(fgg->name, fg->name);
                fgg->size = fg->size;
                fgg->mtime = fg->mtime;
                fgg->crcval = fg->crcval;
                fgg->next = usedgrpcache;
                usedgrpcache = fgg;
                continue;
            }
        }

        {
            int32_t b, fh;
            int32_t crcval;

            fh = openfrompath(sidx->name, BO_RDONLY|BO_BINARY, BS_IREAD);
            if (fh < 0) continue;
            if (fstat(fh, &st)) continue;

            initprintf(" Checksumming %s...", sidx->name);
            crc32init((uint32_t *)&crcval);
            do
            {
                b = read(fh, buf, BUFFER_SIZE);
                if (b > 0) crc32block((uint32_t *)&crcval, (uint8_t *)buf, b);
            }
            while (b == BUFFER_SIZE);
            crc32finish((uint32_t *)&crcval);
            close(fh);
            initprintf(" Done\n");

            grp = (struct grpfile *)Bcalloc(1, sizeof(struct grpfile));
            grp->name = Bstrdup(sidx->name);
            grp->crcval = crcval;
            grp->size = st.st_size;
            grp->next = foundgrps;
            foundgrps = grp;

            fgg = (struct grpcache *)Bcalloc(1, sizeof(struct grpcache));
            Bstrncpy(fgg->name, sidx->name, BMAX_PATH);
            fgg->size = st.st_size;
            fgg->mtime = st.st_mtime;
            fgg->crcval = crcval;
            fgg->next = usedgrpcache;
            usedgrpcache = fgg;
        }
    }

    klistfree(srch);
    FreeGroupsCache();

    if (usedgrpcache)
    {
        int32_t i = 0;
        FILE *fp;
        fp = fopen(GRPCACHEFILE, "wt");
        if (fp)
        {
            for (fg = usedgrpcache; fg; fg=fgg)
            {
                fgg = fg->next;
                fprintf(fp, "\"%s\" %d %d %d\n", fg->name, fg->size, fg->mtime, fg->crcval);
                Bfree(fg);
                i++;
            }
            fclose(fp);
        }
//        initprintf("Found %d recognized GRP %s.\n",i,i>1?"files":"file");
        if (buf)
            Bfree(buf);
        return 0;
    }

    initprintf("Found no recognized game data!\n");

    if (buf)
        Bfree(buf);

    return 0;
}

void FreeGroups(void)
{
    struct grpfile *fg;

    while (foundgrps)
    {
        fg = foundgrps->next;
        Bfree((char *)foundgrps->name);
        Bfree(foundgrps);
        foundgrps = fg;
    }
}

