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

#include "baselayer.h"
#include "cache1d.h"
#include "exhumed.h"
#include "common.h"
#include "compat.h"
#include "crc32.h"
#include "grpscan.h"
#include "scriptfile.h"

#include "vfs.h"

#ifndef EDUKE32_STANDALONE
static void process_vaca13(int32_t crcval);
static void process_vacapp15(int32_t crcval);

// custom GRP support for the startup window, file format reflects the structure below
#define GAMELISTFILE "games.list"
//    name          crc             size      flags                 dependency
static internalgrpinfo_t const internalgrpfiles[] =
{
    { "Powerslave", POWERSLAVE_CRC, 27020745, GAMEFLAG_POWERSLAVE,  0 },
    { "Exhumed",    EXHUMED_CRC,    27108170, GAMEFLAG_EXHUMED,     0 },
};
#endif

struct grpfile_t *foundgrps = NULL;
struct grpinfo_t *listgrps = NULL;

static void LoadList(const char * filename)
{
    scriptfile *script = scriptfile_fromfile(filename);

    if (!script)
        return;

#ifndef EDUKE32_STANDALONE
    scriptfile_addsymbolvalue("GAMEFLAG_POWERSLAVE", GAMEFLAG_POWERSLAVE);
    scriptfile_addsymbolvalue("POWERSLAVE_CRC", POWERSLAVE_CRC);
    scriptfile_addsymbolvalue("EXHUMED_CRC", EXHUMED_CRC);
#endif

    while (!scriptfile_eof(script))
    {
        enum
        {
            T_GRPINFO,
            T_GAMENAME,
            T_CRC,
            T_SIZE,
            T_DEPCRC,
            T_DEFNAME,
            T_FLAGS,
        };

        static const tokenlist profiletokens[] =
        {
            { "grpinfo",            T_GRPINFO },
        };

        int32_t token = getatoken(script,profiletokens,ARRAY_SIZE(profiletokens));
        switch (token)
        {
        case T_GRPINFO:
        {
            int32_t gsize = 0, gcrcval = 0, gflags = GAMEFLAG_POWERSLAVE, gdepcrc = POWERSLAVE_CRC;
            char *gname = NULL, *gdef = NULL;
            char *grpend = NULL;

            static const tokenlist grpinfotokens[] =
            {
                { "name",           T_GAMENAME },
                { "defname",        T_DEFNAME },
                { "crc",            T_CRC },
                { "dependency",     T_DEPCRC },
                { "size",           T_SIZE },
                { "flags",          T_FLAGS },

            };

            if (scriptfile_getbraces(script,&grpend)) break;

            while (script->textptr < grpend)
            {
                int32_t token = getatoken(script,grpinfotokens,ARRAY_SIZE(grpinfotokens));

                switch (token)
                {
                case T_GAMENAME:
                    scriptfile_getstring(script,&gname); break;
                case T_DEFNAME:
                    scriptfile_getstring(script,&gdef); break;

                case T_FLAGS:
                    scriptfile_getsymbol(script,&gflags); gflags &= GAMEFLAGMASK; break;
                case T_DEPCRC:
                    scriptfile_getsymbol(script,&gdepcrc); break;
                case T_CRC:
                    scriptfile_getsymbol(script,&gcrcval); break;
                case T_SIZE:
                    scriptfile_getnumber(script,&gsize); break;
                default:
                    break;
                }

                grpinfo_t * const fg = (grpinfo_t *)Xcalloc(1, sizeof(grpinfo_t));
                fg->next = listgrps;
                listgrps = fg;

                if (gname)
                    fg->name = Xstrdup(gname);

                fg->size = gsize;
                fg->crcval = gcrcval;
                fg->dependency = gdepcrc;
                fg->game = gflags;

                if (gdef)
                    fg->defname = dup_filename(gdef);
            }
            break;
        }

        default:
            break;
        }
    }

    scriptfile_close(script);
    scriptfile_clearsymbols();
}

static void LoadGameList(void)
{
#ifndef EDUKE32_STANDALONE
    for (size_t i = 0; i < ARRAY_SIZE(internalgrpfiles); i++)
    {
        grpinfo_t * const fg = (grpinfo_t *)Xcalloc(1, sizeof(grpinfo_t));

        fg->name = Xstrdup(internalgrpfiles[i].name);
        fg->crcval = internalgrpfiles[i].crcval;
        fg->size = internalgrpfiles[i].size;
        fg->game = internalgrpfiles[i].game;
        fg->dependency = internalgrpfiles[i].dependency;

        fg->next = listgrps;
        listgrps = fg;
    }
#endif

#ifdef USE_PHYSFS
    auto const base = PHYSFS_getBaseDir();
#else
    static char const base[] = "/";
#endif
    CACHE1D_FIND_REC * const srch = klistpath(base, "*.grpinfo", CACHE1D_FIND_FILE);

    for (CACHE1D_FIND_REC *sidx = srch; sidx; sidx = sidx->next)
        LoadList(sidx->name);

    klistfree(srch);
}

static void FreeGameList(void)
{
    while (listgrps)
    {
        Xfree(listgrps->name);
        Xfree(listgrps->defname);

        grpinfo_t * const fg = listgrps->next;
        Xfree(listgrps);
        listgrps = fg;
    }
}


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
        if (scriptfile_getstring(script, &fname)) break;    // filename
        if (scriptfile_getnumber(script, &fsize)) break;    // filesize
        if (scriptfile_getnumber(script, &fmtime)) break;   // modification time
        if (scriptfile_getnumber(script, &fcrcval)) break;  // crc checksum

        fg = (struct grpcache *)Xcalloc(1, sizeof(struct grpcache));
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
    while (grpcache)
    {
        struct grpcache * const fg = grpcache->next;
        Xfree(grpcache);
        grpcache = fg;
    }
}

static void RemoveGroup(grpfile_t *igrp)
{
    for (grpfile_t *prev = NULL, *grp = foundgrps; grp; grp=grp->next)
    {
        if (grp == igrp)
        {
            if (grp == foundgrps)
                foundgrps = grp->next;
            else
                prev->next = grp->next;

            Xfree((char *)grp->filename);
            Xfree(grp);

            return;
        }

        prev = grp;
    }
}

grpfile_t * FindGroup(int32_t crcval)
{
    grpfile_t *grp;

    for (grp = foundgrps; grp; grp=grp->next)
    {
        if (grp->type->crcval == crcval)
            return grp;
    }

    return NULL;
}

#ifndef USE_PHYSFS
static grpinfo_t const * FindGrpInfo(int32_t crcval, int32_t size)
{
    grpinfo_t *grpinfo;

    for (grpinfo = listgrps; grpinfo; grpinfo=grpinfo->next)
    {
        if (grpinfo->crcval == crcval && grpinfo->size == size)
            return grpinfo;
    }

    return NULL;
}

static void ProcessGroups(CACHE1D_FIND_REC *srch)
{
    CACHE1D_FIND_REC *sidx;
    struct grpcache *fg, *fgg;
    char *fn;
    struct Bstat st;

    static constexpr int ReadSize = 65536;

    auto buf = (uint8_t *)Xmalloc(ReadSize);

    for (sidx = srch; sidx; sidx = sidx->next)
    {
        for (fg = grpcache; fg; fg = fg->next)
        {
            if (!Bstrcmp(fg->name, sidx->name)) break;
        }

        if (fg)
        {
            if (findfrompath(sidx->name, &fn)) continue; // failed to resolve the filename
            if (Bstat(fn, &st))
            {
                Xfree(fn);
                continue;
            } // failed to stat the file
            Xfree(fn);
            if (fg->size == (int32_t)st.st_size && fg->mtime == (int32_t)st.st_mtime)
            {
                grpinfo_t const * const grptype = FindGrpInfo(fg->crcval, fg->size);
                if (grptype)
                {
                    grpfile_t * const grp = (grpfile_t *)Xcalloc(1, sizeof(grpfile_t));
                    grp->filename = Xstrdup(sidx->name);
                    grp->type = grptype;
                    grp->next = foundgrps;
                    foundgrps = grp;
                }

                fgg = (struct grpcache *)Xcalloc(1, sizeof(struct grpcache));
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
            int32_t crcval = 0;

            fh = openfrompath(sidx->name, BO_RDONLY|BO_BINARY, BS_IREAD);
            if (fh < 0) continue;
            if (Bfstat(fh, &st)) continue;

            initprintf(" Checksumming %s...", sidx->name);
            do
            {
                b = read(fh, buf, ReadSize);
                if (b > 0) crcval = Bcrc32((uint8_t *)buf, b, crcval);
            }
            while (b == ReadSize);
            close(fh);
            initprintf(" Done\n");

            grpinfo_t const * const grptype = FindGrpInfo(crcval, st.st_size);
            if (grptype)
            {
                grpfile_t * const grp = (grpfile_t *)Xcalloc(1, sizeof(grpfile_t));
                grp->filename = Xstrdup(sidx->name);
                grp->type = grptype;
                grp->next = foundgrps;
                foundgrps = grp;
            }

            fgg = (struct grpcache *)Xcalloc(1, sizeof(struct grpcache));
            Bstrncpy(fgg->name, sidx->name, BMAX_PATH);
            fgg->size = st.st_size;
            fgg->mtime = st.st_mtime;
            fgg->crcval = crcval;
            fgg->next = usedgrpcache;
            usedgrpcache = fgg;
        }
    }

    Xfree(buf);
}
#endif

int32_t ScanGroups(void)
{
#ifndef USE_PHYSFS
    struct grpcache *fg, *fgg;

    initprintf("Searching for game data...\n");

    LoadGameList();
    LoadGroupsCache();

    static char const *extensions[] =
    {
        "*.grp",
        "*.ssi",
        "*.dat",
    };

    for (char const *extension : extensions)
    {
        CACHE1D_FIND_REC *srch = klistpath("/", extension, CACHE1D_FIND_FILE);
        ProcessGroups(srch);
        klistfree(srch);
    }

    FreeGroupsCache();

    for (grpfile_t *grp = foundgrps; grp; grp=grp->next)
    {
        if (grp->type->dependency)
        {
            if (FindGroup(grp->type->dependency) == NULL) // couldn't find dependency
            {
                //initprintf("removing %s\n", grp->name);
                RemoveGroup(grp);
                grp = foundgrps;
                // start from the beginning so we can remove anything that depended on this grp
                continue;
            }
        }
    }

    if (usedgrpcache)
    {
        int32_t i = 0;
        buildvfs_FILE fp = buildvfs_fopen_write(GRPCACHEFILE);
        if (fp)
        {
            for (fg = usedgrpcache; fg; fg=fgg)
            {
                fgg = fg->next;
                fprintf(fp, "\"%s\" %d %d %d\n", fg->name, fg->size, fg->mtime, fg->crcval);
                Xfree(fg);
                i++;
            }
            buildvfs_fclose(fp);
        }
//        initprintf("Found %d recognized GRP %s.\n",i,i>1?"files":"file");

        return 0;
    }

    initprintf("Found no recognized game data!\n");
#endif

    return 0;
}


void FreeGroups(void)
{
    while (foundgrps)
    {
        Xfree((char *)foundgrps->filename);
        grpfile_t * const fg = foundgrps->next;
        Xfree(foundgrps);
        foundgrps = fg;
    }

    FreeGameList();
}
