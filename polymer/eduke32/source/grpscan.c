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
#include "common_game.h"
#include "grpscan.h"

// custom GRP support for the startup window, file format reflects the structure below
#define GAMELISTFILE "games.list"
//    name                                     crc          size      flags                         dependency  scriptname     defname
struct grpfile internalgrpfiles[NUMGRPFILES] =
{
    { "Duke Nukem 3D",                         DUKE13_CRC,  26524524, GAMEFLAG_DUKE,                         0, NULL,          NULL,        NULL },
    { "Duke Nukem 3D (South Korean Censored)", DUKEKR_CRC,  26385383, GAMEFLAG_DUKE,                         0, NULL,          NULL,        NULL },
    { "Duke Nukem 3D: Atomic Edition",         DUKE15_CRC,  44356548, GAMEFLAG_DUKE,                         0, NULL,          NULL,        NULL },
    { "Duke Nukem 3D: Plutonium Pak",          DUKEPP_CRC,  44348015, GAMEFLAG_DUKE,                         0, NULL,          NULL,        NULL },
    { "Duke Nukem 3D Shareware 0.99",          DUKE099_CRC, 9690241,  GAMEFLAG_DUKE|GAMEFLAG_DUKEBETA,       0, NULL,          NULL,        NULL },
    { "Duke Nukem 3D Shareware 1.0",           DUKE10_CRC,  10429258, GAMEFLAG_DUKE|GAMEFLAG_SHAREWARE,      0, NULL,          NULL,        NULL },
    { "Duke Nukem 3D Shareware 1.1",           DUKE11_CRC,  10442980, GAMEFLAG_DUKE|GAMEFLAG_SHAREWARE,      0, NULL,          NULL,        NULL },
    { "Duke Nukem 3D Shareware 1.3D",          DUKESW_CRC,  11035779, GAMEFLAG_DUKE|GAMEFLAG_SHAREWARE,      0, NULL,          NULL,        NULL },
    { "Duke Nukem 3D Mac Demo",                DUKEMD_CRC,  10444391, GAMEFLAG_DUKE|GAMEFLAG_SHAREWARE,      0, NULL,          NULL,        NULL },
    { "Duke it out in D.C.",                   DUKEDC_CRC,  8410183 , GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE15_CRC, NULL,          NULL,        NULL },
    { "Duke Caribbean: Life's a Beach",        DUKECB_CRC,  22213819, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE15_CRC, NULL,          NULL,        NULL },
    { "Duke: Nuclear Winter",                  DUKENW_CRC,  16169365, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE15_CRC, "nwinter.con", NULL,        NULL },
    { "NAM",                                   NAM_CRC,     43448927, GAMEFLAG_NAM,                          0, "nam.con",     "nam.def",   NULL },
    { "NAPALM",                                NAPALM_CRC,  44365728, GAMEFLAG_NAM|GAMEFLAG_NAPALM,          0, "napalm.con",  "nam.def",   NULL },
    { "WWII GI",                               WW2GI_CRC,   77939508, GAMEFLAG_WW2GI|GAMEFLAG_NAM,           0, "ww2gi.con",   "ww2gi.def", NULL },
};
struct grpfile *foundgrps = NULL;
struct grpfile *listgrps = NULL;

static void LoadList(const char * filename)
{
    struct grpfile *fg;

    char *grpend = NULL;

    scriptfile *script = scriptfile_fromfile(filename);

    if (!script)
        return;

    scriptfile_addsymbolvalue("GAMEFLAG_DUKE", GAMEFLAG_DUKE);
    scriptfile_addsymbolvalue("GAMEFLAG_ADDON", GAMEFLAG_DUKE|GAMEFLAG_ADDON);
    scriptfile_addsymbolvalue("DUKE15_CRC", DUKE15_CRC);
    scriptfile_addsymbolvalue("DUKE13_CRC", DUKE13_CRC);
    scriptfile_addsymbolvalue("DUKEDC_CRC", DUKEDC_CRC);
    scriptfile_addsymbolvalue("DUKECB_CRC", DUKECB_CRC);
    scriptfile_addsymbolvalue("DUKENW_CRC", DUKENW_CRC);

    while (!scriptfile_eof(script))
    {
        enum
        {
            T_GRPINFO,
            T_GAMENAME,
            T_CRC,
            T_SIZE,
            T_DEPCRC,
            T_SCRIPTNAME,
            T_DEFNAME,
            T_FLAGS,
        };

        static const tokenlist profiletokens[] =
        {
            { "grpinfo",            T_GRPINFO },
        };

        int32_t token = getatoken(script,profiletokens,sizeof(profiletokens)/sizeof(tokenlist));
        switch (token)
        {
        case T_GRPINFO:
        {
            int32_t gsize = 0, gcrcval = 0, gflags = GAMEFLAG_DUKE, gdepcrc = DUKE15_CRC;
            char *gname = NULL, *gscript = NULL, *gdef = NULL;

            static const tokenlist grpinfotokens[] =
            {
                { "name",           T_GAMENAME },
                { "scriptname",     T_SCRIPTNAME },
                { "defname",        T_DEFNAME },
                { "crc",            T_CRC },
                { "dependency",     T_DEPCRC },
                { "size",           T_SIZE },
                { "flags",          T_FLAGS },

            };

            if (scriptfile_getbraces(script,&grpend)) break;

            while (script->textptr < grpend)
            {
                int32_t token = getatoken(script,grpinfotokens,sizeof(grpinfotokens)/sizeof(tokenlist));

                switch (token)
                {
                case T_GAMENAME:
                    scriptfile_getstring(script,&gname); break;
                case T_SCRIPTNAME:
                    scriptfile_getstring(script,&gscript); break;
                case T_DEFNAME:
                    scriptfile_getstring(script,&gdef); break;

                case T_FLAGS:
                    scriptfile_getsymbol(script,&gflags); break;
                case T_DEPCRC:
                    scriptfile_getsymbol(script,&gdepcrc); break;
                case T_CRC:
                    scriptfile_getsymbol(script,&gcrcval); break;
                case T_SIZE:
                    scriptfile_getnumber(script,&gsize); break;
                default:
                    break;
                }

                fg = (struct grpfile *)Bcalloc(1, sizeof(struct grpfile));
                fg->next = listgrps;
                listgrps = fg;

                if (gname)
                    fg->name = Bstrdup(gname);

                fg->size = gsize;
                fg->crcval = gcrcval;
                fg->dependency = gdepcrc;
                fg->game = gflags;

                if (gscript)
                    fg->scriptname = dup_filename(gscript);

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
    struct grpfile *fg;
    CACHE1D_FIND_REC *srch, *sidx;
    
    int32_t i;

    for (i = 0; i<NUMGRPFILES; i++)
    {
        fg = (struct grpfile *)Bcalloc(1, sizeof(struct grpfile));

        fg->name = Bstrdup(internalgrpfiles[i].name);
        fg->crcval = internalgrpfiles[i].crcval;
        fg->size = internalgrpfiles[i].size;
        fg->game = internalgrpfiles[i].game;
        fg->dependency = internalgrpfiles[i].dependency;

        if (internalgrpfiles[i].scriptname)
            fg->scriptname = dup_filename(internalgrpfiles[i].scriptname);

        if (internalgrpfiles[i].defname)
            fg->defname = dup_filename(internalgrpfiles[i].defname);

        fg->next = listgrps;
        listgrps = fg;
    }

    srch = klistpath("/", "*.grpinfo", CACHE1D_FIND_FILE);

    for (sidx = srch; sidx; sidx = sidx->next)
        LoadList(srch->name);

    klistfree(srch);
}

static void FreeGameList(void)
{
    struct grpfile *fg;

    while (listgrps)
    {
        fg = listgrps->next;
        Bfree(listgrps->name);

        if (listgrps->scriptname)
            Bfree(listgrps->scriptname);

        if (listgrps->defname)
            Bfree(listgrps->defname);

        Bfree(listgrps);
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

        fg = (struct grpcache *)Bcalloc(1, sizeof(struct grpcache));
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

void RemoveGroup(int32_t crcval)
{
    struct grpfile *grp;

    for (grp = foundgrps; grp; grp=grp->next)
    {
        if (grp->crcval == crcval)
        {
            if (grp == foundgrps)
                foundgrps = grp->next;
            else
            {
                struct grpfile *fg;

                for (fg = foundgrps; fg; fg=fg->next)
                {
                    if (fg->next == grp)
                    {
                        fg->next = grp->next;
                        break;
                    }
                }
            }

            Bfree((char *)grp->name);
            Bfree(grp);

            RemoveGroup(crcval);

            break;
        }
    }
}

struct grpfile * FindGroup(int32_t crcval)
{
    struct grpfile *grp;

    for (grp = foundgrps; grp; grp=grp->next)
    {
        if (grp->crcval == crcval)
            return grp;
    }

    return NULL;
}

int32_t ScanGroups(void)
{
    CACHE1D_FIND_REC *srch, *sidx;
    struct grpcache *fg, *fgg;
    struct grpfile *grp;
    char *fn;
    struct Bstat st;
#define BUFFER_SIZE (1024 * 1024 * 8)
    uint8_t *buf = (uint8_t *)Bmalloc(BUFFER_SIZE);

    if (!buf)
    {
        initprintf("Error allocating %d byte buffer to scan GRPs!\n", BUFFER_SIZE);
        return 0;
    }

    initprintf("Searching for game data...\n");

    LoadGameList();
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
            if (findfrompath(sidx->name, &fn)) continue; // failed to resolve the filename
            if (Bstat(fn, &st))
            {
                Bfree(fn);
                continue;
            } // failed to stat the file
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
            if (Bfstat(fh, &st)) continue;

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

    for (grp = foundgrps; grp; /*grp=grp->next*/)
    {
        struct grpfile *igrp;
        for (igrp = listgrps; igrp; igrp=igrp->next)
            if (grp->crcval == igrp->crcval) break;

        if (igrp == NULL)
        {
            grp = grp->next;
            continue;
        }

        if (igrp->dependency)
        {
            struct grpfile *depgrp;

            //initprintf("found grp with dep\n");
            for (depgrp = foundgrps; depgrp; depgrp=depgrp->next)
                if (depgrp->crcval == igrp->dependency) break;

            if (depgrp == NULL || depgrp->crcval != igrp->dependency) // couldn't find dependency
            {
                //initprintf("removing %s\n", grp->name);
                RemoveGroup(igrp->crcval);
                grp = foundgrps;
                continue;
            }
        }

        if (igrp->game && !grp->game)
            grp->game = igrp->game;

        grp=grp->next;
    }

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

        Bfree(buf);
        return 0;
    }

    initprintf("Found no recognized game data!\n");

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

    FreeGameList();
}

