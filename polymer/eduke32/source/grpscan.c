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

#include "compat.h"
#include "baselayer.h"

#include "scriptfile.h"
#include "cache1d.h"
#include "crc32.h"

#include "duke3d.h"
#include "common_game.h"
#include "grpscan.h"

static void process_vaca13(void);
static void process_vacapp15(void);

// custom GRP support for the startup window, file format reflects the structure below
#define GAMELISTFILE "games.list"
//    name                                     crc          size      flags                         dependency  scriptname     defname           postprocessing
struct grpfile internalgrpfiles[] =
{
    { "Duke Nukem 3D",                         DUKE13_CRC,  26524524, GAMEFLAG_DUKE,                         0, NULL, NULL, NULL, NULL },
    { "Duke Nukem 3D (South Korean Censored)", DUKEKR_CRC,  26385383, GAMEFLAG_DUKE,                         0, NULL, NULL, NULL, NULL },
    { "Duke Nukem 3D: Atomic Edition",         DUKE15_CRC,  44356548, GAMEFLAG_DUKE,                         0, NULL, NULL, NULL, NULL },
    { "Duke Nukem 3D: Plutonium Pak",          DUKEPP_CRC,  44348015, GAMEFLAG_DUKE,                         0, NULL, NULL, NULL, NULL },
    { "Duke Nukem 3D Shareware 0.99",          DUKE099_CRC, 9690241,  GAMEFLAG_DUKE|GAMEFLAG_DUKEBETA,       0, NULL, NULL, NULL, NULL },
    { "Duke Nukem 3D Shareware 1.0",           DUKE10_CRC,  10429258, GAMEFLAG_DUKE|GAMEFLAG_SHAREWARE,      0, NULL, NULL, NULL, NULL },
    { "Duke Nukem 3D Shareware 1.1",           DUKE11_CRC,  10442980, GAMEFLAG_DUKE|GAMEFLAG_SHAREWARE,      0, NULL, NULL, NULL, NULL },
    { "Duke Nukem 3D Shareware 1.3D",          DUKESW_CRC,  11035779, GAMEFLAG_DUKE|GAMEFLAG_SHAREWARE,      0, NULL, NULL, NULL, NULL },
    { "Duke Nukem 3D Mac Demo",                DUKEMD_CRC,  10444391, GAMEFLAG_DUKE|GAMEFLAG_SHAREWARE,      0, NULL, NULL, NULL, NULL },
    { "Duke it out in D.C. (1.3D)",            DUKEDC13_CRC, 7926624, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE13_CRC, NULL, NULL, NULL, NULL },
    { "Duke it out in D.C.",                   DUKEDCPP_CRC, 8225517, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE15_CRC, NULL, NULL, NULL, NULL },
    { "Duke it out in D.C.",                   DUKEDC_CRC,  8410183,  GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE15_CRC, NULL, NULL, NULL, NULL },
    { "Duke Caribbean: Life's a Beach (1.3D)", VACA13_CRC,  23559381, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE13_CRC, NULL, NULL, process_vaca13, NULL },
    { "Duke Caribbean: Life's a Beach (PPak)", VACAPP_CRC,  22551333, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKEPP_CRC, NULL, NULL, process_vacapp15, NULL },
    { "Duke Caribbean: Life's a Beach",        VACA15_CRC,  22521880, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE15_CRC, NULL, NULL, process_vacapp15, NULL },
    { "Duke Caribbean: Life's a Beach",        DUKECB_CRC,  22213819, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE15_CRC, NULL, NULL, NULL, NULL },
    { "Duke: Nuclear Winter",                  DUKENW_CRC,  16169365, GAMEFLAG_DUKE|GAMEFLAG_ADDON|GAMEFLAG_NWINTER, DUKE15_CRC, "NWINTER.CON", NULL, NULL, NULL },
    { "NAM",                                   NAM_CRC,     43448927, GAMEFLAG_NAM,                          0, NULL, NULL, NULL, NULL },
    { "NAPALM",                                NAPALM_CRC,  44365728, GAMEFLAG_NAM|GAMEFLAG_NAPALM,          0, NULL, NULL, NULL, NULL },
    { "WWII GI",                               WW2GI_CRC,   77939508, GAMEFLAG_WW2GI|GAMEFLAG_NAM,           0, NULL, NULL, NULL, NULL },
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
    scriptfile_addsymbolvalue("GAMEFLAG_NAM", GAMEFLAG_NAM);
    scriptfile_addsymbolvalue("GAMEFLAG_NAPALM", GAMEFLAG_NAM|GAMEFLAG_NAPALM);
    scriptfile_addsymbolvalue("GAMEFLAG_WW2GI", GAMEFLAG_NAM|GAMEFLAG_WW2GI);
    scriptfile_addsymbolvalue("DUKE15_CRC", DUKE15_CRC);
    scriptfile_addsymbolvalue("DUKEPP_CRC", DUKEPP_CRC);
    scriptfile_addsymbolvalue("DUKE13_CRC", DUKE13_CRC);
    scriptfile_addsymbolvalue("DUKEDC13_CRC", DUKEDC13_CRC);
    scriptfile_addsymbolvalue("DUKEDCPP_CRC", DUKEDCPP_CRC);
    scriptfile_addsymbolvalue("DUKEDC_CRC", DUKEDC_CRC);
    scriptfile_addsymbolvalue("VACA13_CRC", VACA13_CRC);
    scriptfile_addsymbolvalue("VACAPP_CRC", VACAPP_CRC);
    scriptfile_addsymbolvalue("VACA15_CRC", VACA15_CRC);
    scriptfile_addsymbolvalue("DUKECB_CRC", DUKECB_CRC);
    scriptfile_addsymbolvalue("DUKENW_CRC", DUKENW_CRC);
    scriptfile_addsymbolvalue("NAM_CRC", NAM_CRC);
    scriptfile_addsymbolvalue("NAPALM_CRC", NAPALM_CRC);
    scriptfile_addsymbolvalue("WW2GI_CRC", WW2GI_CRC);

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

        int32_t token = getatoken(script,profiletokens,ARRAY_SIZE(profiletokens));
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
                int32_t token = getatoken(script,grpinfotokens,ARRAY_SIZE(grpinfotokens));

                switch (token)
                {
                case T_GAMENAME:
                    scriptfile_getstring(script,&gname); break;
                case T_SCRIPTNAME:
                    scriptfile_getstring(script,&gscript); break;
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

                fg = (struct grpfile *)Xcalloc(1, sizeof(struct grpfile));
                fg->next = listgrps;
                listgrps = fg;

                if (gname)
                    fg->name = Xstrdup(gname);

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

    for (size_t i = 0; i < ARRAY_SIZE(internalgrpfiles); i++)
    {
        fg = (struct grpfile *)Xcalloc(1, sizeof(struct grpfile));

        fg->name = Xstrdup(internalgrpfiles[i].name);
        fg->crcval = internalgrpfiles[i].crcval;
        fg->size = internalgrpfiles[i].size;
        fg->game = internalgrpfiles[i].game;
        fg->dependency = internalgrpfiles[i].dependency;

        if (internalgrpfiles[i].scriptname)
            fg->scriptname = dup_filename(internalgrpfiles[i].scriptname);

        if (internalgrpfiles[i].defname)
            fg->defname = dup_filename(internalgrpfiles[i].defname);

        fg->postprocessing = internalgrpfiles[i].postprocessing;

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

static void ProcessGroups(CACHE1D_FIND_REC *srch)
{
    CACHE1D_FIND_REC *sidx;
    struct grpcache *fg, *fgg;
    struct grpfile *grp;
    char *fn;
    struct Bstat st;

#define BUFFER_SIZE (1024 * 1024 * 8)
    uint8_t *buf = (uint8_t *)Xmalloc(BUFFER_SIZE);

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
            if (fg->size == (int32_t)st.st_size && fg->mtime == (int32_t)st.st_mtime)
            {
                grp = (struct grpfile *)Xcalloc(1, sizeof(struct grpfile));
                grp->name = Xstrdup(sidx->name);
                grp->crcval = fg->crcval;
                grp->size = fg->size;
                grp->next = foundgrps;
                foundgrps = grp;

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
                b = read(fh, buf, BUFFER_SIZE);
                if (b > 0) crcval = Bcrc32((uint8_t *)buf, b, crcval);
            }
            while (b == BUFFER_SIZE);
            close(fh);
            initprintf(" Done\n");

            grp = (struct grpfile *)Xcalloc(1, sizeof(struct grpfile));
            grp->name = Xstrdup(sidx->name);
            grp->crcval = crcval;
            grp->size = st.st_size;
            grp->next = foundgrps;
            foundgrps = grp;

            fgg = (struct grpcache *)Xcalloc(1, sizeof(struct grpcache));
            Bstrncpy(fgg->name, sidx->name, BMAX_PATH);
            fgg->size = st.st_size;
            fgg->mtime = st.st_mtime;
            fgg->crcval = crcval;
            fgg->next = usedgrpcache;
            usedgrpcache = fgg;
        }
    }

    Bfree(buf);
}

int32_t ScanGroups(void)
{
    CACHE1D_FIND_REC *srch;
    struct grpcache *fg, *fgg;
    struct grpfile *grp;

    initprintf("Searching for game data...\n");

    LoadGameList();
    LoadGroupsCache();

    srch = klistpath("/", "*.grp", CACHE1D_FIND_FILE);
    ProcessGroups(srch);
    klistfree(srch);

    srch = klistpath("/", "*.ssi", CACHE1D_FIND_FILE);
    ProcessGroups(srch);
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

        return 0;
    }

    initprintf("Found no recognized game data!\n");

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

static void process_vaca13(void)
{
    krename("ADDREE.COV", "ADDREE.VOC");
    krename("BALLBOOM.COV", "BALLBOOM.VOC");
    krename("BARMUSIC.COV", "BARMUSIC.VOC");
    krename("BCHBALL.COV", "BCHBALL.VOC");
    krename("BOING.COV", "BOING.VOC");
    krename("CHACHA.COV", "CHACHA.VOC");
    krename("CHAINDRV.COV", "CHAINDRV.VOC");
    krename("CHEAP01.COV", "CHEAP01.VOC");
    krename("CHEER.COV", "CHEER.VOC");
    krename("CHNSQRT.COV", "CHNSQRT.VOC");
    krename("COCOANUT.COV", "COCOANUT.VOC");
    krename("CRUSH2.COV", "CRUSH2.VOC");
    krename("DEFLATE2.COV", "DEFLATE2.VOC");
    krename("DRAGHURT.COV", "DRAGHURT.VOC");
    krename("DRAGROAM.COV", "DRAGROAM.VOC");
    krename("DRAGSHOT.COV", "DRAGSHOT.VOC");
    krename("DUKE01.COV", "DUKE01.VOC");
    krename("ELEV1.COV", "ELEV1.VOC");
    krename("GMEOVR05.COV", "GMEOVR05.VOC");
    krename("GULLDIE.COV", "GULLDIE.VOC");
    krename("GULLHURT.COV", "GULLHURT.VOC");
    krename("GULLROAM.COV", "GULLROAM.VOC");
    krename("GULLSHIT.COV", "GULLSHIT.VOC");
    krename("HELP04.COV", "HELP04.VOC");
    krename("ICECONCH.COV", "ICECONCH.VOC");
    krename("IDLEBOAT.COV", "IDLEBOAT.VOC");
    krename("KICKHEAD.COV", "KICKHEAD.VOC");
    krename("LANI05.COV", "LANI05.VOC");
    krename("LANI08.COV", "LANI08.VOC");
    krename("LANIDUK2.COV", "LANIDUK2.VOC");
    krename("MUSCLE01.COV", "MUSCLE01.VOC");
    krename("MUSCLE04.COV", "MUSCLE04.VOC");
    krename("MUZAK.COV", "MUZAK.VOC");
    krename("PINEFALL.COV", "PINEFALL.VOC");
    krename("POINT07.COV", "POINT07.VOC");
    krename("POINT08.COV", "POINT08.VOC");
    krename("RADIO.COV", "RADIO.VOC");
    krename("RUIN01.COV", "RUIN01.VOC");
    krename("SCREAM.COV", "SCREAM.VOC");
    krename("SCREAM04.COV", "SCREAM04.VOC");
    krename("SCREAM9.COV", "SCREAM9.VOC");
    krename("SHIPHORN.COV", "SHIPHORN.VOC");
    krename("SNGLGULL.COV", "SNGLGULL.VOC");
    krename("SQRT4.COV", "SQRT4.VOC");
    krename("SQUIRT1.COV", "SQUIRT1.VOC");
    krename("SSCOOL1.COV", "SSCOOL1.VOC");
    krename("SSCOOL2.COV", "SSCOOL2.VOC");
    krename("SSCOOL3.COV", "SSCOOL3.VOC");
    krename("SSDIE1.COV", "SSDIE1.VOC");
    krename("SSDIE2.COV", "SSDIE2.VOC");
    krename("SSNORM01.COV", "SSNORM01.VOC");
    krename("SSNORM02.COV", "SSNORM02.VOC");
    krename("SSNORM03.COV", "SSNORM03.VOC");
    krename("SSNORM04.COV", "SSNORM04.VOC");
    krename("SSNORM05.COV", "SSNORM05.VOC");
    krename("SSNORM06.COV", "SSNORM06.VOC");
    krename("SSNORM07.COV", "SSNORM07.VOC");
    krename("SSNORM08.COV", "SSNORM08.VOC");
    krename("SSNORM10.COV", "SSNORM10.VOC");
    krename("SSNORM11.COV", "SSNORM11.VOC");
    krename("SSNORM12.COV", "SSNORM12.VOC");
    krename("SSNORM13.COV", "SSNORM13.VOC");
    krename("SSNORM14.COV", "SSNORM14.VOC");
    krename("SSNORM15.COV", "SSNORM15.VOC");
    krename("SSNORM16.COV", "SSNORM16.VOC");
    krename("SSNORM17.COV", "SSNORM17.VOC");
    krename("SSNORM18.COV", "SSNORM18.VOC");
    krename("SSNORM19.COV", "SSNORM19.VOC");
    krename("SSNORM20.COV", "SSNORM20.VOC");
    krename("SSTAUNT1.COV", "SSTAUNT1.VOC");
    krename("SSTAUNT2.COV", "SSTAUNT2.VOC");
    krename("SSTAUNT3.COV", "SSTAUNT3.VOC");
    krename("SSTAUNT4.COV", "SSTAUNT4.VOC");
    krename("SSTAUNT5.COV", "SSTAUNT5.VOC");
    krename("SSTAUNT6.COV", "SSTAUNT6.VOC");
    krename("SSTAUNT7.COV", "SSTAUNT7.VOC");
    krename("SSTAUNT8.COV", "SSTAUNT8.VOC");
    krename("SURF.COV", "SURF.VOC");
    krename("TAN01.COV", "TAN01.VOC");
    krename("TAN04.COV", "TAN04.VOC");
    krename("VINESNAP.COV", "VINESNAP.VOC");
    krename("VOODRUMS.COV", "VOODRUMS.VOC");
    krename("WIND54.COV", "WIND54.VOC");
    krename("DOOMSDAY.DIM", "DOOMSDAY.MID");
    krename("DUKE-O.DIM", "DUKE-O.MID");
    krename("IRIEPRTY.DIM", "IRIEPRTY.MID");
    krename("JUNGVEIN.DIM", "JUNGVEIN.MID");
    krename("PRTYCRUZ.DIM", "PRTYCRUZ.MID");
    krename("SOL-MAN1.DIM", "SOL-MAN1.MID");
    krename("CINEOV3.MNA", "CINEOV3.ANM");
    krename("DUKETEAM.MNA", "DUKETEAM.ANM");
    krename("BEACHBAB.NOC", "BEACHBAB.CON");
    krename("BEACHBAL.NOC", "BEACHBAL.CON");
    krename("BEACHBTH.NOC", "BEACHBTH.CON");
    krename("DEFS.NOC", "DEFS.CON");
    krename("DRAGON.NOC", "DRAGON.CON");
    krename("GAME.NOC", "GAME.CON");
    krename("SEAGULL.NOC", "SEAGULL.CON");
    krename("SOUNDS.NOC", "SOUNDS.CON");
    krename("USER.NOC", "USER.CON");
    krename("DEMO1.OMD", "DEMO1.DMO");
    krename("DEMO2.OMD", "DEMO2.DMO");
    krename("DEMO3.OMD", "DEMO3.DMO");
    krename("VACA1.PAM", "VACA1.MAP");
    krename("VACA2.PAM", "VACA2.MAP");
    krename("VACA3.PAM", "VACA3.MAP");
    krename("VACA4.PAM", "VACA4.MAP");
    krename("VACA5.PAM", "VACA5.MAP");
    krename("VACA6.PAM", "VACA6.MAP");
    krename("VACA7.PAM", "VACA7.MAP");
    krename("VACADM1.PAM", "VACADM1.MAP");
    krename("VACADM2.PAM", "VACADM2.MAP");
    krename("VACADM3.PAM", "VACADM3.MAP");
    krename("VACADM4.PAM", "VACADM4.MAP");
    krename("VACASL.PAM", "VACASL.MAP");
    krename("TILES000.TRA", "TILES000.ART");
    krename("TILES001.TRA", "TILES001.ART");
    krename("TILES003.TRA", "TILES003.ART");
    krename("TILES005.TRA", "TILES005.ART");
    krename("TILES006.TRA", "TILES006.ART");
    krename("TILES007.TRA", "TILES007.ART");
    krename("TILES008.TRA", "TILES008.ART");
    krename("TILES009.TRA", "TILES009.ART");
    krename("TILES010.TRA", "TILES010.ART");
    krename("TILES012.TRA", "TILES012.ART");
    krename("TILES014.TRA", "TILES014.ART");
}

static void process_vacapp15(void)
{
    krename("DEFS.NOC", "DEFS.CON");
    krename("GAME.NOC", "GAME.CON");
    krename("USER.NOC", "USER.CON");
    krename("DEMO1.OMD", "DEMO1.DMO");
    krename("DEMO2.OMD", "DEMO2.DMO");
    krename("DEMO3.OMD", "DEMO3.DMO");

    initgroupfile("VACATION.PRG");
}
