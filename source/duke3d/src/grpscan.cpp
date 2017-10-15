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

#ifndef EDUKE32_STANDALONE
static void process_vaca13(int32_t crcval);
static void process_vacapp15(int32_t crcval);

// custom GRP support for the startup window, file format reflects the structure below
#define GAMELISTFILE "games.list"
//    name                                     crc          size      flags                         dependency  scriptname     postprocessing
static internalgrpinfo_t const internalgrpfiles[] =
{
    { "Duke Nukem 3D",                         DUKE13_CRC,  26524524, GAMEFLAG_DUKE,                         0, NULL, NULL},
    { "Duke Nukem 3D (South Korean Censored)", DUKEKR_CRC,  26385383, GAMEFLAG_DUKE,                         0, NULL, NULL},
    { "Duke Nukem 3D: Atomic Edition",         DUKE15_CRC,  44356548, GAMEFLAG_DUKE,                         0, NULL, NULL},
    { "Duke Nukem 3D: Atomic Edition (WT)",    DUKEWT_CRC,  44356548, GAMEFLAG_DUKE,                         0, NULL, NULL},
    { "Duke Nukem 3D: Plutonium Pak",          DUKEPP_CRC,  44348015, GAMEFLAG_DUKE,                         0, NULL, NULL},
    { "Duke Nukem 3D Shareware 0.99",          DUKE099_CRC, 9690241,  GAMEFLAG_DUKE|GAMEFLAG_DUKEBETA,       0, NULL, NULL},
    { "Duke Nukem 3D Shareware 1.0",           DUKE10_CRC,  10429258, GAMEFLAG_DUKE|GAMEFLAG_SHAREWARE,      0, NULL, NULL},
    { "Duke Nukem 3D Shareware 1.1",           DUKE11_CRC,  10442980, GAMEFLAG_DUKE|GAMEFLAG_SHAREWARE,      0, NULL, NULL},
    { "Duke Nukem 3D Shareware 1.3D",          DUKESW_CRC,  11035779, GAMEFLAG_DUKE|GAMEFLAG_SHAREWARE,      0, NULL, NULL},
    { "Duke Nukem 3D Mac Demo",                DUKEMD_CRC,  10444391, GAMEFLAG_DUKE|GAMEFLAG_SHAREWARE,      0, NULL, NULL},
    { "Duke Nukem 3D MacUser Demo",            DUKEMD2_CRC, 10628573, GAMEFLAG_DUKE|GAMEFLAG_SHAREWARE,      0, NULL, NULL },
    { "Duke it out in D.C. (1.3D)",            DUKEDC13_CRC, 7926624, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE13_CRC, NULL, NULL},
    { "Duke it out in D.C.",                   DUKEDCPP_CRC, 8225517, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE15_CRC, NULL, NULL},
    { "Duke it out in D.C.",                   DUKEDC_CRC,  8410183,  GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE15_CRC, NULL, NULL},
    { "Duke it out in D.C.",                   (int32_t) 0x39A692BF,  8410187, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE15_CRC, "DUKEDC.CON", NULL},
    { "Duke Caribbean: Life's a Beach (1.3D)", VACA13_CRC,  23559381, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE13_CRC, NULL, process_vaca13},
    { "Duke Caribbean: Life's a Beach (PPak)", VACAPP_CRC,  22551333, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKEPP_CRC, NULL, process_vacapp15},
    { "Duke Caribbean: Life's a Beach",        VACA15_CRC,  22521880, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE15_CRC, NULL, process_vacapp15},
    { "Duke Caribbean: Life's a Beach",        DUKECB_CRC,  22213819, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE15_CRC, NULL, NULL},
    { "Duke Caribbean: Life's a Beach",        (int32_t) 0x65B5F690, 22397273, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE15_CRC, "VACATION.CON", NULL},
    { "Duke: Nuclear Winter",                  DUKENW_CRC,  16169365, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE15_CRC, "NWINTER.CON", NULL},
    { "Duke!ZONE II (1.3D)",                   DZ2_13_CRC,  26135388, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE13_CRC, "DZ-GAME.CON", NULL},
    { "Duke!ZONE II",                          DZ2_PP_CRC,  44100411, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE15_CRC, "DZ-GAME.CON", NULL},
    { "Duke!ZONE II",                          (int32_t) 0x1E9516F1,  3186656, GAMEFLAG_DUKE|GAMEFLAG_ADDON, DUKE15_CRC, "DZ-GAME.CON", NULL},
    { "NAM",                                   NAM_CRC,     43448927, GAMEFLAG_NAM,                          0, NULL, NULL},
    { "NAPALM",                                NAPALM_CRC,  44365728, GAMEFLAG_NAM|GAMEFLAG_NAPALM,          0, NULL, NULL},
    { "WWII GI",                               WW2GI_CRC,   77939508, GAMEFLAG_WW2GI,                        0, NULL, NULL},
    { "Platoon Leader",                        PLATOONL_CRC, 37852572, GAMEFLAG_WW2GI|GAMEFLAG_ADDON,        WW2GI_CRC, "PLATOONL.DEF", NULL},
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
    scriptfile_addsymbolvalue("DZ2_13_CRC", DZ2_13_CRC);
    scriptfile_addsymbolvalue("DZ2_PP_CRC", DZ2_PP_CRC);
    scriptfile_addsymbolvalue("NAM_CRC", NAM_CRC);
    scriptfile_addsymbolvalue("NAPALM_CRC", NAPALM_CRC);
    scriptfile_addsymbolvalue("WW2GI_CRC", WW2GI_CRC);
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
            char *grpend = NULL;

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

                grpinfo_t * const fg = (grpinfo_t *)Xcalloc(1, sizeof(grpinfo_t));
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
#ifndef EDUKE32_STANDALONE
    for (size_t i = 0; i < ARRAY_SIZE(internalgrpfiles); i++)
    {
        grpinfo_t * const fg = (grpinfo_t *)Xcalloc(1, sizeof(grpinfo_t));

        fg->name = Xstrdup(internalgrpfiles[i].name);
        fg->crcval = internalgrpfiles[i].crcval;
        fg->size = internalgrpfiles[i].size;
        fg->game = internalgrpfiles[i].game;
        fg->dependency = internalgrpfiles[i].dependency;

        if (internalgrpfiles[i].scriptname)
            fg->scriptname = dup_filename(internalgrpfiles[i].scriptname);

        fg->postprocessing = internalgrpfiles[i].postprocessing;

        fg->next = listgrps;
        listgrps = fg;
    }
#endif

    CACHE1D_FIND_REC * const srch = klistpath("/", "*.grpinfo", CACHE1D_FIND_FILE);

    for (CACHE1D_FIND_REC *sidx = srch; sidx; sidx = sidx->next)
        LoadList(sidx->name);

    klistfree(srch);
}

static void FreeGameList(void)
{
    while (listgrps)
    {
        Bfree(listgrps->name);
        Bfree(listgrps->scriptname);
        Bfree(listgrps->defname);

        grpinfo_t * const fg = listgrps->next;
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
    while (grpcache)
    {
        struct grpcache * const fg = grpcache->next;
        Bfree(grpcache);
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

            Bfree((char *)grp->filename);
            Bfree(grp);

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
                b = read(fh, buf, BUFFER_SIZE);
                if (b > 0) crcval = Bcrc32((uint8_t *)buf, b, crcval);
            }
            while (b == BUFFER_SIZE);
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

    Bfree(buf);
}

int32_t ScanGroups(void)
{
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
    while (foundgrps)
    {
        Bfree((char *)foundgrps->filename);
        grpfile_t * const fg = foundgrps->next;
        Bfree(foundgrps);
        foundgrps = fg;
    }

    FreeGameList();
}

#ifndef EDUKE32_STANDALONE
static void process_vaca13(int32_t crcval)
{
    krename(crcval, 0, "ADDREE.VOC");
    krename(crcval, 1, "BALLBOOM.VOC");
    krename(crcval, 2, "BARMUSIC.VOC");
    krename(crcval, 3, "BCHBALL.VOC");
    krename(crcval, 4, "BOING.VOC");
    krename(crcval, 5, "CHACHA.VOC");
    krename(crcval, 6, "CHAINDRV.VOC");
    krename(crcval, 7, "CHEAP01.VOC");
    krename(crcval, 8, "CHEER.VOC");
    krename(crcval, 9, "CHNSQRT.VOC");
    krename(crcval, 10, "COCOANUT.VOC");
    krename(crcval, 11, "CRUSH2.VOC");
    krename(crcval, 12, "DEFLATE2.VOC");
    krename(crcval, 13, "DRAGHURT.VOC");
    krename(crcval, 14, "DRAGROAM.VOC");
    krename(crcval, 15, "DRAGSHOT.VOC");
    krename(crcval, 16, "DUKE01.VOC");
    krename(crcval, 17, "ELEV1.VOC");
    krename(crcval, 18, "GMEOVR05.VOC");
    krename(crcval, 19, "GULLDIE.VOC");
    krename(crcval, 20, "GULLHURT.VOC");
    krename(crcval, 21, "GULLROAM.VOC");
    krename(crcval, 22, "GULLSHIT.VOC");
    krename(crcval, 23, "HELP04.VOC");
    krename(crcval, 24, "ICECONCH.VOC");
    krename(crcval, 25, "IDLEBOAT.VOC");
    krename(crcval, 26, "KICKHEAD.VOC");
    krename(crcval, 27, "LANI05.VOC");
    krename(crcval, 28, "LANI08.VOC");
    krename(crcval, 29, "LANIDUK2.VOC");
    krename(crcval, 30, "MUSCLE01.VOC");
    krename(crcval, 31, "MUSCLE04.VOC");
    krename(crcval, 32, "MUZAK.VOC");
    krename(crcval, 33, "PINEFALL.VOC");
    krename(crcval, 34, "POINT07.VOC");
    krename(crcval, 35, "POINT08.VOC");
    krename(crcval, 36, "RADIO.VOC");
    krename(crcval, 37, "RUIN01.VOC");
    krename(crcval, 38, "SCREAM.VOC");
    krename(crcval, 39, "SCREAM04.VOC");
    krename(crcval, 40, "SCREAM9.VOC");
    krename(crcval, 41, "SHIPHORN.VOC");
    krename(crcval, 42, "SNGLGULL.VOC");
    krename(crcval, 43, "SQRT4.VOC");
    krename(crcval, 44, "SQUIRT1.VOC");
    krename(crcval, 45, "SSCOOL1.VOC");
    krename(crcval, 46, "SSCOOL2.VOC");
    krename(crcval, 47, "SSCOOL3.VOC");
    krename(crcval, 48, "SSDIE1.VOC");
    krename(crcval, 49, "SSDIE2.VOC");
    krename(crcval, 50, "SSNORM01.VOC");
    krename(crcval, 51, "SSNORM02.VOC");
    krename(crcval, 52, "SSNORM03.VOC");
    krename(crcval, 53, "SSNORM04.VOC");
    krename(crcval, 54, "SSNORM05.VOC");
    krename(crcval, 55, "SSNORM06.VOC");
    krename(crcval, 56, "SSNORM07.VOC");
    krename(crcval, 57, "SSNORM08.VOC");
    krename(crcval, 58, "SSNORM10.VOC");
    krename(crcval, 59, "SSNORM11.VOC");
    krename(crcval, 60, "SSNORM12.VOC");
    krename(crcval, 61, "SSNORM13.VOC");
    krename(crcval, 62, "SSNORM14.VOC");
    krename(crcval, 63, "SSNORM15.VOC");
    krename(crcval, 64, "SSNORM16.VOC");
    krename(crcval, 65, "SSNORM17.VOC");
    krename(crcval, 66, "SSNORM18.VOC");
    krename(crcval, 67, "SSNORM19.VOC");
    krename(crcval, 68, "SSNORM20.VOC");
    krename(crcval, 69, "SSTAUNT1.VOC");
    krename(crcval, 70, "SSTAUNT2.VOC");
    krename(crcval, 71, "SSTAUNT3.VOC");
    krename(crcval, 72, "SSTAUNT4.VOC");
    krename(crcval, 73, "SSTAUNT5.VOC");
    krename(crcval, 74, "SSTAUNT6.VOC");
    krename(crcval, 75, "SSTAUNT7.VOC");
    krename(crcval, 76, "SSTAUNT8.VOC");
    krename(crcval, 77, "SURF.VOC");
    krename(crcval, 78, "TAN01.VOC");
    krename(crcval, 79, "TAN04.VOC");
    krename(crcval, 80, "VINESNAP.VOC");
    krename(crcval, 81, "VOODRUMS.VOC");
    krename(crcval, 82, "WIND54.VOC");
    krename(crcval, 83, "DOOMSDAY.MID");
    krename(crcval, 84, "DUKE-O.MID");
    krename(crcval, 85, "IRIEPRTY.MID");
    krename(crcval, 86, "JUNGVEIN.MID");
    krename(crcval, 87, "PRTYCRUZ.MID");
    krename(crcval, 88, "SOL-MAN1.MID");
    krename(crcval, 90, "CINEOV3.ANM");
    krename(crcval, 91, "DUKETEAM.ANM");
    krename(crcval, 92, "BEACHBAB.CON");
    krename(crcval, 93, "BEACHBAL.CON");
    krename(crcval, 94, "BEACHBTH.CON");
    krename(crcval, 95, "DEFS.CON");
    krename(crcval, 96, "DRAGON.CON");
    krename(crcval, 97, "GAME.CON");
    krename(crcval, 98, "SEAGULL.CON");
    krename(crcval, 99, "SOUNDS.CON");
    krename(crcval, 100, "USER.CON");
    krename(crcval, 101, "DEMO1.DMO");
    krename(crcval, 102, "DEMO2.DMO");
    krename(crcval, 103, "DEMO3.DMO");
    krename(crcval, 104, "VACA1.MAP");
    krename(crcval, 105, "VACA2.MAP");
    krename(crcval, 106, "VACA3.MAP");
    krename(crcval, 107, "VACA4.MAP");
    krename(crcval, 108, "VACA5.MAP");
    krename(crcval, 109, "VACA6.MAP");
    krename(crcval, 110, "VACA7.MAP");
    krename(crcval, 111, "VACADM1.MAP");
    krename(crcval, 112, "VACADM2.MAP");
    krename(crcval, 113, "VACADM3.MAP");
    krename(crcval, 114, "VACADM4.MAP");
    krename(crcval, 115, "VACASL.MAP");
    krename(crcval, 120, "TILES000.ART");
    krename(crcval, 121, "TILES001.ART");
    krename(crcval, 122, "TILES003.ART");
    krename(crcval, 123, "TILES005.ART");
    krename(crcval, 124, "TILES006.ART");
    krename(crcval, 125, "TILES007.ART");
    krename(crcval, 126, "TILES008.ART");
    krename(crcval, 127, "TILES009.ART");
    krename(crcval, 128, "TILES010.ART");
    krename(crcval, 129, "TILES012.ART");
    krename(crcval, 130, "TILES014.ART");
}

static void process_vacapp15(int32_t crcval)
{
    krename(crcval, 5, "DEFS.CON");
    krename(crcval, 6, "GAME.CON");
    krename(crcval, 7, "USER.CON");
    krename(crcval, 8, "DEMO1.DMO");
    krename(crcval, 9, "DEMO2.DMO");
    krename(crcval, 10, "DEMO3.DMO");

    initgroupfile("VACATION.PRG");
}
#endif
