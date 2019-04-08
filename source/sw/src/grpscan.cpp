//-------------------------------------------------------------------------
/*
 Copyright (C) 2007 Jonathon Fowler <jf@jonof.id.au>

 This file is part of JFShadowWarrior

 Shadow Warrior is free software; you can redistribute it and/or
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

#include "build.h"
#include "baselayer.h"

#include "scriptfile.h"
#include "cache1d.h"
#include "crc32.h"

#include "grpscan.h"

internalgrpfile grpfiles[numgrpfiles] =
{
    { "Registered Version",     0x7545319Fu, 47536148 },
    { "Shareware Version",      0x08A7FA1Fu, 26056769 },
    { "Wanton Destruction (Addon)", 0xA9AAA7B7u, 48698128 },
};
grpfile *foundgrps = NULL;

#define GRPCACHEFILE "grpfiles.cache"
static struct grpcache
{
    struct grpcache *next;
    char name[BMAX_PATH+1];
    int size;
    int mtime;
    unsigned int crcval;
} *grpcache = NULL, *usedgrpcache = NULL;

static int LoadGroupsCache(void)
{
    struct grpcache *fg;

    int fsize, fmtime, fcrcval;
    char *fname;

    scriptfile *script;

    script = scriptfile_fromfile(GRPCACHEFILE);
    if (!script) return -1;

    while (!scriptfile_eof(script))
    {
        if (scriptfile_getstring(script, &fname)) break;    // filename
        if (scriptfile_getnumber(script, &fsize)) break;    // filesize
        if (scriptfile_getnumber(script, &fmtime)) break;   // modification time
        if (scriptfile_getnumber(script, &fcrcval)) break;  // crc checksum

        fg = (struct grpcache*)calloc(1, sizeof(struct grpcache));
        fg->next = grpcache;
        grpcache = fg;

        strncpy(fg->name, fname, BMAX_PATH);
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
        free(grpcache);
        grpcache = fg;
    }
}

int ScanGroups(void)
{
    CACHE1D_FIND_REC *srch, *sidx;
    struct grpcache *fg, *fgg;
    struct grpfile *grp;
    char *fn;
    struct Bstat st;

    buildputs("Scanning for GRP files...\n");

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
            if (findfrompath(sidx->name, &fn)) continue;    // failed to resolve the filename
            if (Bstat(fn, &st)) { free(fn); continue; } // failed to stat the file
            free(fn);
            if (fg->size == st.st_size && fg->mtime == st.st_mtime)
            {
                grp = (struct grpfile *)calloc(1, sizeof(struct grpfile));
                grp->name = strdup(sidx->name);
                grp->crcval = fg->crcval;
                grp->size = fg->size;
                grp->next = foundgrps;
                foundgrps = grp;

                fgg = (struct grpcache *)calloc(1, sizeof(struct grpcache));
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
            int b, fh;
            unsigned int crcval = 0;
            unsigned char buf[16*512];

            fh = openfrompath(sidx->name, BO_RDONLY|BO_BINARY, BS_IREAD);
            if (fh < 0) continue;
            if (fstat(fh, &st)) continue;

            buildprintf(" Checksumming %s...", sidx->name);
            do
            {
                b = read(fh, buf, sizeof(buf));
                if (b > 0) crcval = Bcrc32(buf, b, crcval);
            }
            while (b == sizeof(buf));
            close(fh);
            buildputs(" Done\n");

            grp = (struct grpfile *)calloc(1, sizeof(struct grpfile));
            grp->name = strdup(sidx->name);
            grp->crcval = crcval;
            grp->size = st.st_size;
            grp->next = foundgrps;
            foundgrps = grp;

            fgg = (struct grpcache *)calloc(1, sizeof(struct grpcache));
            strncpy(fgg->name, sidx->name, BMAX_PATH);
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
        FILE *fp;
        fp = fopen(GRPCACHEFILE, "wt");
        if (fp)
        {
            for (fg = usedgrpcache; fg; fg=fgg)
            {
                fgg = fg->next;
                fprintf(fp, "\"%s\" %d %d %d\n", fg->name, fg->size, fg->mtime, fg->crcval);
                free(fg);
            }
            fclose(fp);
        }
    }

    return 0;
}

void FreeGroups(void)
{
    struct grpfile *fg;

    while (foundgrps)
    {
        fg = foundgrps->next;
        free(foundgrps->name);
        free(foundgrps);
        foundgrps = fg;
    }
}

