/*
 * Definitions file parser for Build
 * by Jonathon Fowler (jf@jonof.id.au)
 * Remixed substantially by Ken Silverman
 * See the included license file "BUILDLIC.TXT" for license info.
 */

#include "build.h"
#include "compat.h"
#include "engine_priv.h"
#include "baselayer.h"
#include "scriptfile.h"
#include "cache1d.h"
#include "kplib.h"
#include "lz4.h"
#include "common.h"
#include "mdsprite.h"  // md3model_t
#include "colmatch.h"

#ifdef USE_OPENGL
# include "hightile.h"
#endif

enum scripttoken_t
{
    T_INCLUDE = 0,
    T_DEFINE,
    T_DEFINETEXTURE,
    T_DEFINESKYBOX,
    T_DEFINETINT,
    T_DEFINEMODEL,
    T_DEFINEMODELFRAME,
    T_DEFINEMODELANIM,
    T_DEFINEMODELSKIN,
    T_SELECTMODELSKIN,
    T_DEFINEVOXEL,
    T_DEFINEVOXELTILES,
    T_MODEL,
    T_FILE,
    T_SCALE,
    T_SHADE,
    T_FRAME,
    T_SMOOTHDURATION,
    T_ANIM,
    T_SKIN,
    T_SURF,
    T_TILE,
    T_TILE0,
    T_TILE1,
    T_FRAME0,
    T_FRAME1,
    T_FPS,
    T_FLAGS,
    T_PAL,
    T_BASEPAL,
    T_DETAIL,
    T_GLOW,
    T_SPECULAR,
    T_NORMAL,
    T_PARAM,
    T_HUD,
    T_XADD,
    T_YADD,
    T_ZADD,
    T_ANGADD,
    T_FOV,
    T_FLIPPED,
    T_HIDE,
    T_NOBOB,
    T_NODEPTH,
    T_VOXEL,
    T_SKYBOX,
    T_FRONT,T_RIGHT,T_BACK,T_LEFT,T_TOP,T_BOTTOM,
    T_HIGHPALOOKUP,
    T_TINT,
    T_MAKEPALOOKUP, T_REMAPPAL, T_REMAPSELF,
    T_NOFLOORPAL, T_FLOORPAL,
    T_RED,T_GREEN,T_BLUE,
    T_TEXTURE,T_ALPHACUT,T_XSCALE,T_YSCALE,T_SPECPOWER,T_SPECFACTOR,T_NOCOMPRESS,T_NODOWNSIZE,
    T_FORCEFILTER,
    T_ARTQUALITY,
    T_ORIGSIZEX,T_ORIGSIZEY,
    T_UNDEFMODEL,T_UNDEFMODELRANGE,T_UNDEFMODELOF,T_UNDEFTEXTURE,T_UNDEFTEXTURERANGE,
    T_ALPHAHACK,T_ALPHAHACKRANGE,
    T_SPRITECOL,T_2DCOL,T_2DCOLIDXRANGE,
    T_FOGPAL,
    T_LOADGRP,
    T_DUMMYTILE,T_DUMMYTILERANGE,
    T_SETUPTILE,T_SETUPTILERANGE,
    T_UNDEFINETILE,T_UNDEFINETILERANGE,
    T_ANIMTILERANGE,
    T_CACHESIZE,
    T_IMPORTTILE,
    T_MUSIC,T_ID,T_SOUND,
    T_TILEFROMTEXTURE, T_XOFFSET, T_YOFFSET, T_TEXHITSCAN, T_NOFULLBRIGHT,
    T_ARTFILE,
    T_INCLUDEDEFAULT,
    T_ANIMSOUNDS,
    T_CUTSCENE,
    T_NOFLOORPALRANGE,
    T_TEXHITSCANRANGE,
    T_NOFULLBRIGHTRANGE,
    T_MAPINFO, T_MAPFILE, T_MAPTITLE, T_MAPMD4, T_MHKFILE,
    T_ECHO,
    T_GLOBALFLAGS,
    T_COPYTILE,
    T_GLOBALGAMEFLAGS,
    T_MULTIPSKY, T_HORIZFRAC, T_LOGNUMTILES,
    T_BASEPALETTE, T_PALOOKUP, T_BLENDTABLE,
    T_RAW, T_OFFSET, T_SHIFTLEFT, T_NOSHADES, T_COPY,
    T_NUMALPHATABS,
    T_UNDEF,
    T_UNDEFBASEPALETTERANGE, T_UNDEFPALOOKUPRANGE, T_UNDEFBLENDTABLERANGE,
    T_GLBLEND, T_FORWARD, T_REVERSE, T_BOTH, T_SRC, T_DST, T_ALPHA,
    T_ZERO, T_ONE,
    T_SRC_COLOR, T_ONE_MINUS_SRC_COLOR,
    T_SRC_ALPHA, T_ONE_MINUS_SRC_ALPHA,
    T_DST_ALPHA, T_ONE_MINUS_DST_ALPHA,
    T_DST_COLOR, T_ONE_MINUS_DST_COLOR,
    T_SHADERED, T_SHADEGREEN, T_SHADEBLUE,
};

static int32_t lastmodelid = -1, lastvoxid = -1, modelskin = -1, lastmodelskin = -1, seenframe = 0;
static int32_t nextvoxid = 0;
static char *faketilebuffer = NULL;
static int32_t faketilebuffersiz = 0;

#ifdef USE_OPENGL
extern uint8_t alphahackarray[MAXTILES];
#endif

static const char *skyfaces[6] =
{
    "front face", "right face", "back face",
    "left face", "top face", "bottom face"
};

static int32_t defsparser(scriptfile *script);

static void defsparser_include(const char *fn, const scriptfile *script, const char *cmdtokptr)
{
    scriptfile *included;

    included = scriptfile_fromfile(fn);
    if (EDUKE32_PREDICT_FALSE(!included))
    {
        if (!cmdtokptr)
            initprintf("Warning: Failed including %s as module\n", fn);
        else
            initprintf("Warning: Failed including %s on line %s:%d\n",
                       fn, script->filename,scriptfile_getlinum(script,cmdtokptr));
    }
    else
    {
        if (!cmdtokptr)
        {
            flushlogwindow = 1;
            initprintf("Loading module \"%s\"\n",fn);
            flushlogwindow = 0;
        }

        defsparser(included);
        scriptfile_close(included);
    }
}


static int32_t check_tile_range(const char *defcmd, int32_t *tilebeg, int32_t *tileend,
                                const scriptfile *script, const char *cmdtokptr)
{
    if (EDUKE32_PREDICT_FALSE(*tileend < *tilebeg))
    {
        initprintf("Warning: %s: backwards tile range on line %s:%d\n", defcmd,
                   script->filename, scriptfile_getlinum(script,cmdtokptr));
        swaplong(tilebeg, tileend);
    }

    if (EDUKE32_PREDICT_FALSE((unsigned)*tilebeg >= MAXUSERTILES || (unsigned)*tileend >= MAXUSERTILES))
    {
        initprintf("Error: %s: Invalid tile range on line %s:%d\n", defcmd,
                   script->filename, scriptfile_getlinum(script,cmdtokptr));
        return 1;
    }

    return 0;
}

static int32_t check_tile(const char *defcmd, int32_t tile, const scriptfile *script,
                          const char *cmdtokptr)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)tile >= MAXUSERTILES))
    {
        initprintf("Error: %s: Invalid tile number on line %s:%d\n", defcmd,
                   script->filename, scriptfile_getlinum(script,cmdtokptr));
        return 1;
    }

    return 0;
}

static void tile_from_truecolpic(int32_t tile, const palette_t *picptr, int32_t alphacut)
{
    vec2s_t const siz = tilesiz[tile];
    int32_t i, j, tsiz = siz.x * siz.y;

    maybe_grow_buffer(&faketilebuffer, &faketilebuffersiz, tsiz);

    getclosestcol_flush();

    for (j = 0; j < siz.y; ++j)
    {
        int const ofs = j * siz.x;
        for (i = 0; i < siz.x; ++i)
        {
            palette_t const *const col = &picptr[ofs + i];
            faketilebuffer[(i * siz.y) + j] =
            (col->f < alphacut) ? 255 : getclosestcol_lim(col->b, col->g, col->r, 254);
        }
    }

    E_CreateFakeTile(tile, tsiz, faketilebuffer);
}

static int32_t Defs_LoadTileIntoBuffer(int32_t const tile)
{
    vec2s_t const siz = tilesiz[tile];
    int32_t const tsiz = siz.x * siz.y;

    if (EDUKE32_PREDICT_FALSE(tilesiz[tile].x <= 0 || tilesiz[tile].y <= 0))
        return 0;

    maybe_grow_buffer(&faketilebuffer, &faketilebuffersiz, tsiz);

    E_LoadTileIntoBuffer(tile, tsiz, faketilebuffer);

    return tsiz;
}

static void Defs_ApplyPaletteToTileBuffer(int32_t const tsiz, int32_t const pal)
{
    for (bssize_t i = 0; i < tsiz; i++)
        faketilebuffer[i] = palookup[pal][faketilebuffer[i]];
}

static int32_t Defs_ImportTileFromTexture(char const * const fn, int32_t const tile, int32_t const alphacut, int32_t istexture)
{
    if (check_file_exist(fn))
        return -1;

    int32_t xsiz = 0, ysiz = 0;
    palette_t *picptr = NULL;

    int32_t const length = kpzbufload(fn);
#ifdef WITHKPLIB
    kpzdecode(length, (intptr_t *)&picptr, &xsiz, &ysiz);
#endif

    if (!picptr)
    {
        int32_t const artstatus = E_CheckUnitArtFileHeader((uint8_t *)kpzbuf, length);
        if (artstatus < 0)
            return artstatus<<8;

        Bmemcpy(&picanm[tile], &kpzbuf[20], sizeof(picanm_t));
        E_ConvertARTv1picanmToMemory(tile);

        int32_t const xsiz = B_LITTLE16(B_UNBUF16(&kpzbuf[16]));
        int32_t const ysiz = B_LITTLE16(B_UNBUF16(&kpzbuf[18]));

        if (EDUKE32_PREDICT_FALSE(xsiz <= 0 || ysiz <= 0))
        {
            E_UndefineTile(tile);
            return 2;
        }

        set_tilesiz(tile, xsiz, ysiz);
        int32_t const dasiz = xsiz * ysiz;

        if (EDUKE32_PREDICT_FALSE(ARTv1_UNITOFFSET + dasiz > length))
        {
            E_CreateDummyTile(tile);
            return 3;
        }

        E_CreateFakeTile(tile, dasiz, &kpzbuf[ARTv1_UNITOFFSET]);

#ifdef USE_OPENGL
        if (istexture)
            hicsetsubsttex(tile, 0, fn, (float)(255-alphacut) * (1.f/255.f), 1.0f, 1.0f, 1.0f, 1.0f, HICR_ARTIMMUNITY);
#endif

        return 1;
    }

    if (EDUKE32_PREDICT_FALSE(xsiz <= 0 || ysiz <= 0))
        return -2;

    if (!(paletteloaded & PALETTE_MAIN))
        return -3;

    set_tilesiz(tile, xsiz, ysiz);

    tile_from_truecolpic(tile, picptr, alphacut);

    Bfree(picptr);

#ifdef USE_OPENGL
    if (istexture)
        hicsetsubsttex(tile, 0, fn, (float)(255-alphacut) * (1.f/255.f), 1.0f, 1.0f, 1.0, 1.0, HICR_ARTIMMUNITY);
#else
    UNREFERENCED_PARAMETER(istexture);
#endif

    return 0;
}

#undef USE_DEF_PROGRESS
#if defined _WIN32 || defined HAVE_GTK2
# define USE_DEF_PROGRESS
#endif

static int32_t defsparser(scriptfile *script)
{
    int32_t tokn;
    char *cmdtokptr;
#ifdef USE_DEF_PROGRESS
    static uint32_t iter = 0;
#endif

    static const tokenlist basetokens[] =
    {
        { "include",         T_INCLUDE          },
        { "#include",        T_INCLUDE          },
        { "includedefault",  T_INCLUDEDEFAULT   },
        { "#includedefault", T_INCLUDEDEFAULT   },
        { "define",          T_DEFINE           },
        { "#define",         T_DEFINE           },

        // deprecated style
        { "definetexture",   T_DEFINETEXTURE    },
        { "defineskybox",    T_DEFINESKYBOX     },
        { "definetint",      T_DEFINETINT       },
        { "definemodel",     T_DEFINEMODEL      },
        { "definemodelframe",T_DEFINEMODELFRAME },
        { "definemodelanim", T_DEFINEMODELANIM  },
        { "definemodelskin", T_DEFINEMODELSKIN  },
        { "selectmodelskin", T_SELECTMODELSKIN  },
        { "definevoxel",     T_DEFINEVOXEL      },
        { "definevoxeltiles",T_DEFINEVOXELTILES },

        // new style
        { "model",           T_MODEL            },
        { "voxel",           T_VOXEL            },
        { "skybox",          T_SKYBOX           },
        { "highpalookup",    T_HIGHPALOOKUP     },
        { "tint",            T_TINT             },
        { "makepalookup",    T_MAKEPALOOKUP     },
        { "texture",         T_TEXTURE          },
        { "tile",            T_TEXTURE          },
        { "music",           T_MUSIC            },
        { "sound",           T_SOUND            },
        { "animsounds",      T_ANIMSOUNDS       },  // dummy
        { "cutscene",        T_CUTSCENE         },
        { "nofloorpalrange", T_NOFLOORPALRANGE  },
        { "texhitscanrange", T_TEXHITSCANRANGE  },
        { "nofullbrightrange", T_NOFULLBRIGHTRANGE },
        // other stuff
        { "undefmodel",      T_UNDEFMODEL       },
        { "undefmodelrange", T_UNDEFMODELRANGE  },
        { "undefmodelof",    T_UNDEFMODELOF     },
        { "undeftexture",    T_UNDEFTEXTURE     },
        { "undeftexturerange", T_UNDEFTEXTURERANGE },
        { "alphahack",	     T_ALPHAHACK 		},
        { "alphahackrange",  T_ALPHAHACKRANGE 	},
        { "spritecol",	     T_SPRITECOL 		},
        { "2dcol",	     	 T_2DCOL 			},
        { "2dcolidxrange",   T_2DCOLIDXRANGE	},
        { "fogpal",	     	 T_FOGPAL	 		},
        { "loadgrp",     	 T_LOADGRP	 		},
        { "dummytile",     	 T_DUMMYTILE		},
        { "dummytilerange",  T_DUMMYTILERANGE   },
        { "setuptile",       T_SETUPTILE        },
        { "setuptilerange",  T_SETUPTILERANGE   },
        { "undefinetile",    T_UNDEFINETILE		},
        { "undefinetilerange", T_UNDEFINETILERANGE },
        { "animtilerange",   T_ANIMTILERANGE    },
        { "cachesize",       T_CACHESIZE        },
        { "dummytilefrompic",T_IMPORTTILE       },
        { "tilefromtexture", T_TILEFROMTEXTURE  },
        { "artfile",         T_ARTFILE          },
        { "mapinfo",         T_MAPINFO          },
        { "echo",            T_ECHO             },
        { "globalflags",     T_GLOBALFLAGS      },
        { "copytile",        T_COPYTILE         },
        { "globalgameflags", T_GLOBALGAMEFLAGS  },  // dummy
        { "multipsky",       T_MULTIPSKY        },
        { "basepalette",     T_BASEPALETTE      },
        { "palookup",        T_PALOOKUP         },
        { "blendtable",      T_BLENDTABLE       },
        { "numalphatables",  T_NUMALPHATABS     },
        { "undefbasepaletterange", T_UNDEFBASEPALETTERANGE },
        { "undefpalookuprange", T_UNDEFPALOOKUPRANGE },
        { "undefblendtablerange", T_UNDEFBLENDTABLERANGE },
    };

    while (1)
    {
#ifdef USE_DEF_PROGRESS
        if (++iter >= 50)
        {
            flushlogwindow = 1;
            initprintf(".");
            flushlogwindow = 0;
            iter = 0;
        }
#endif
        handleevents();
        if (quitevent) return 0;
        tokn = getatoken(script,basetokens,ARRAY_SIZE(basetokens));
        cmdtokptr = script->ltextptr;
        switch (tokn)
        {
        case T_ERROR:
            initprintf("Error on line %s:%d.\n", script->filename,scriptfile_getlinum(script,cmdtokptr));
            break;
        case T_EOF:
            return 0;
        case T_INCLUDE:
        {
            char *fn;
            if (!scriptfile_getstring(script,&fn))
                defsparser_include(fn, script, cmdtokptr);
            break;
        }
        case T_INCLUDEDEFAULT:
        {
            defsparser_include(G_DefaultDefFile(), script, cmdtokptr);
            break;
        }
        case T_DEFINE:
        {
            char *name;
            int32_t number;

            if (scriptfile_getstring(script,&name)) break;
            if (scriptfile_getsymbol(script,&number)) break;

            if (EDUKE32_PREDICT_FALSE(scriptfile_addsymbolvalue(name,number) < 0))
                initprintf("Warning: Symbol %s was NOT redefined to %d on line %s:%d\n",
                           name,number,script->filename,scriptfile_getlinum(script,cmdtokptr));
            break;
        }

        // OLD (DEPRECATED) DEFINITION SYNTAX
        case T_DEFINETEXTURE:
        {
            int32_t tile,pal,fnoo;
            char *fn;

            if (scriptfile_getsymbol(script,&tile)) break;
            if (scriptfile_getsymbol(script,&pal))  break;
            if (scriptfile_getnumber(script,&fnoo)) break; //x-center
            if (scriptfile_getnumber(script,&fnoo)) break; //y-center
            if (scriptfile_getnumber(script,&fnoo)) break; //x-size
            if (scriptfile_getnumber(script,&fnoo)) break; //y-size
            if (scriptfile_getstring(script,&fn))  break;

            if (check_file_exist(fn))
                break;

#ifdef USE_OPENGL
            hicsetsubsttex(tile,pal,fn,-1.0,1.0,1.0,1.0,1.0,0);
#endif
        }
        break;
        case T_DEFINESKYBOX:
        {
            int32_t tile,pal,i;
            char *fn[6],happy=1;

            if (scriptfile_getsymbol(script,&tile)) break;
            if (scriptfile_getsymbol(script,&pal)) break;
            if (scriptfile_getsymbol(script,&i)) break; //future expansion
            for (i=0; i<6; i++)
            {
                if (scriptfile_getstring(script,&fn[i])) break; //grab the 6 faces

                if (check_file_exist(fn[i]))
                    happy = 0;
            }
            if (i < 6 || !happy) break;
#ifdef USE_OPENGL
            hicsetskybox(tile,pal,fn, 0);
#endif
        }
        break;
        case T_DEFINETINT:
        {
            int32_t pal, r,g,b,sr,sg,sb,f;

            if (scriptfile_getsymbol(script,&pal)) break;
            if (scriptfile_getnumber(script,&r)) break;
            if (scriptfile_getnumber(script,&g)) break;
            if (scriptfile_getnumber(script,&b)) break;
            if (scriptfile_getnumber(script,&sr)) break;
            if (scriptfile_getnumber(script,&sg)) break;
            if (scriptfile_getnumber(script,&sb)) break;
            if (scriptfile_getnumber(script,&f)) break; //effects
#ifdef USE_OPENGL
            hicsetpalettetint(pal,r,g,b,sr,sg,sb,f);
#endif
        }
        break;
        case T_ALPHAHACK:
        {
            int32_t tile;
            double alpha;

            if (scriptfile_getsymbol(script,&tile)) break;
            if (scriptfile_getdouble(script,&alpha)) break;
#ifdef USE_OPENGL
            if ((uint32_t)tile < MAXTILES)
                alphahackarray[tile] = Blrintf(alpha * (float)UINT8_MAX);
#endif
        }
        break;
        case T_ALPHAHACKRANGE:
        {
            int32_t tilenume1,tilenume2;
            double alpha;
#ifdef USE_OPENGL
            int32_t i;
#endif

            if (scriptfile_getsymbol(script,&tilenume1)) break;
            if (scriptfile_getsymbol(script,&tilenume2)) break;
            if (scriptfile_getdouble(script,&alpha)) break;

            if (check_tile_range("alphahackrange", &tilenume1, &tilenume2, script, cmdtokptr))
                break;

#ifdef USE_OPENGL
            for (i=tilenume1; i<=tilenume2; i++)
                alphahackarray[i] = Blrintf(alpha * (float)UINT8_MAX);
#endif
        }
        break;
        case T_SPRITECOL:
        {
            int32_t tile,col,col2;

            if (scriptfile_getsymbol(script,&tile)) break;
            if (scriptfile_getnumber(script,&col)) break;
            if (scriptfile_getnumber(script,&col2)) break;
            if ((uint32_t)tile < MAXTILES)
            {
                spritecol2d[tile][0] = col;
                spritecol2d[tile][1] = col2;
            }
        }
        break;
        case T_2DCOL:
        {
            int32_t col,b,g,r;

            if (scriptfile_getnumber(script,&col)) break;
            if (scriptfile_getnumber(script,&r)) break;
            if (scriptfile_getnumber(script,&g)) break;
            if (scriptfile_getnumber(script,&b)) break;

            if ((unsigned)col < 256)
            {
                r = clamp(r, 0, 63);
                g = clamp(g, 0, 63);
                b = clamp(b, 0, 63);

                vgapal16[col*4+0] = b<<2; // blue
                vgapal16[col*4+1] = g<<2; // green
                vgapal16[col*4+2] = r<<2; // red
            }
        }
        break;
        case T_2DCOLIDXRANGE:  // NOTE: takes precedence over 2dcol, see InitCustomColors()
        {
            int32_t col, idx, idxend;

            if (scriptfile_getnumber(script,&col)) break;
            if (scriptfile_getnumber(script,&idx)) break;
            if (scriptfile_getnumber(script,&idxend)) break;

            while ((unsigned)col < 256 && idx <= idxend)
                editorcolors[col++] = idx++;
        }
        break;
        case T_FOGPAL:
        {
            int32_t p,r,g,b;

            if (scriptfile_getnumber(script,&p)) break;
            if (scriptfile_getnumber(script,&r)) break;
            if (scriptfile_getnumber(script,&g)) break;
            if (scriptfile_getnumber(script,&b)) break;

            r = clamp(r, 0, 63);
            g = clamp(g, 0, 63);
            b = clamp(b, 0, 63);

            makepalookup(p, NULL, r<<2, g<<2, b<<2, 1);
        }
        break;
        case T_NOFLOORPALRANGE:
        {
            int32_t b,e,i;

            if (scriptfile_getnumber(script,&b)) break;
            if (scriptfile_getnumber(script,&e)) break;

            b = max(b, 1);
            e = min(e, MAXPALOOKUPS-1);

            for (i=b; i<=e; i++)
                g_noFloorPal[i] = 1;
        }
        break;
        case T_LOADGRP:
        {
            char *bs;
            scriptfile_getstring(script,&bs);
        }
        break;
        case T_CACHESIZE:
        {
            int32_t j;

            if (scriptfile_getnumber(script,&j)) break;
        }
        break;
        case T_ARTFILE:
        {
            char *blockend, *fn = NULL;
            int32_t tile = -1, havetile = 0;

            static const tokenlist artfiletokens[] =
            {
                { "file",            T_FILE },
                { "tile",            T_TILE },
            };

            if (scriptfile_getbraces(script,&blockend)) break;
            while (script->textptr < blockend)
            {
                int32_t token = getatoken(script,artfiletokens,ARRAY_SIZE(artfiletokens));
                switch (token)
                {
                case T_FILE:
                    scriptfile_getstring(script,&fn);
                    break;
                case T_TILE:
                    havetile = 1;
                    scriptfile_getsymbol(script,&tile);
                    break;
                default:
                    break;
                }
            }

            if (EDUKE32_PREDICT_FALSE(!fn))
            {
                initprintf("Error: missing 'file name' for artfile definition near line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }

            int32_t const fil = kopen4load(fn, 0);
            if (fil == -1)
                break;

            artheader_t local;
            int32_t headerval = E_ReadArtFileHeader(fil, fn, &local);
            if (headerval != 0)
            {
                kclose(fil);
                break;
            }

            if (havetile)
            {
                if (!check_tile("artfile", tile, script, cmdtokptr))
                {
                    local.tilestart = tile;
                    local.tileend = tile + local.numtiles - 1;
                }
            }

            E_ReadArtFileTileInfo(fil, &local);
            E_ReadArtFileIntoFakeData(fil, &local);

            kclose(fil);
        }
        break;
        case T_SETUPTILE:
        {
            int32_t tile, tmp;

            if (scriptfile_getsymbol(script,&tile)) break;
            if (check_tile("setuptile", tile, script, cmdtokptr))
                break;
            if (scriptfile_getsymbol(script,&tmp)) break;  // XXX
            h_xsize[tile] = tmp;
            if (scriptfile_getsymbol(script,&tmp)) break;
            h_ysize[tile] = tmp;
            if (scriptfile_getsymbol(script,&tmp)) break;
            h_xoffs[tile]=tmp;
            if (scriptfile_getsymbol(script,&tmp)) break;
            h_yoffs[tile]=tmp;
            break;
        }
        case T_SETUPTILERANGE:
        {
            int32_t tile1,tile2,xsiz,ysiz,xoffs,yoffs,i;

            if (scriptfile_getnumber(script,&tile1)) break;
            if (scriptfile_getnumber(script,&tile2)) break;
            if (scriptfile_getnumber(script,&xsiz)) break;
            if (scriptfile_getnumber(script,&ysiz)) break;
            if (scriptfile_getsymbol(script,&xoffs)) break;
            if (scriptfile_getsymbol(script,&yoffs)) break;

            if (check_tile_range("setuptilerange", &tile1, &tile2, script, cmdtokptr))
                break;

            for (i=tile1; i<=tile2; i++)
            {
                h_xsize[i] = xsiz;
                h_ysize[i] = ysiz;
                h_xoffs[i] = xoffs;
                h_yoffs[i] = yoffs;
            }

            break;
        }
        case T_ANIMTILERANGE:
        {
            int32_t tile1, tile2, spd, type;

            if (scriptfile_getsymbol(script,&tile1)) break;
            if (scriptfile_getsymbol(script,&tile2)) break;
            if (scriptfile_getsymbol(script,&spd)) break;
            if (scriptfile_getsymbol(script,&type)) break;

            if (check_tile("animtilerange", tile1, script, cmdtokptr))
                break;
            if (check_tile("animtilerange", tile2, script, cmdtokptr))
                break;

            spd = clamp(spd, 0, 15);
            if (EDUKE32_PREDICT_FALSE(type&~3))
            {
                initprintf("Error: animtilerange: animation type must be 0, 1, 2 or 3 on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }

            int32_t num = tile2-tile1;
            if (type == 3 && tile1 > tile2) // PICANM_ANIMTYPE_BACK
                num = -num;

            if (EDUKE32_PREDICT_FALSE((unsigned)num > 255))
            {
                initprintf("Error: animtilerange: tile difference can be at most 255 on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }

            // set anim speed
            picanm[tile1].sf &= ~PICANM_ANIMSPEED_MASK;
            picanm[tile1].sf |= spd;
            // set anim type
            picanm[tile1].sf &= ~PICANM_ANIMTYPE_MASK;
            picanm[tile1].sf |= type<<PICANM_ANIMTYPE_SHIFT;
            // set anim number
            picanm[tile1].num = num;

            break;
        }
        case T_TILEFROMTEXTURE:
        {
            char *texturetokptr = script->ltextptr, *textureend, *fn = NULL;
            int32_t tile = -1;
            int32_t alphacut = 255, flags = 0;
            int32_t havexoffset = 0, haveyoffset = 0;
            int32_t xoffset = 0, yoffset = 0;
            int32_t istexture = 0;

            static const tokenlist tilefromtexturetokens[] =
            {
                { "file",            T_FILE },
                { "name",            T_FILE },
                { "alphacut",        T_ALPHACUT },
                { "xoffset",         T_XOFFSET },
                { "xoff",            T_XOFFSET },
                { "yoffset",         T_YOFFSET },
                { "yoff",            T_YOFFSET },
                { "texhitscan",      T_TEXHITSCAN },
                { "nofullbright",    T_NOFULLBRIGHT },
                { "texture",         T_TEXTURE },
            };

            if (scriptfile_getsymbol(script,&tile)) break;
            if (scriptfile_getbraces(script,&textureend)) break;
            while (script->textptr < textureend)
            {
                int32_t token = getatoken(script,tilefromtexturetokens,ARRAY_SIZE(tilefromtexturetokens));
                switch (token)
                {
                case T_FILE:
                    scriptfile_getstring(script,&fn);
                    break;
                case T_ALPHACUT:
                    scriptfile_getsymbol(script,&alphacut);
                    alphacut = clamp(alphacut, 0, 255);
                    break;
                case T_XOFFSET:
                    havexoffset = 1;
                    scriptfile_getsymbol(script,&xoffset);
                    xoffset = clamp(xoffset, -128, 127);
                    break;
                case T_YOFFSET:
                    haveyoffset = 1;
                    scriptfile_getsymbol(script,&yoffset);
                    yoffset = clamp(yoffset, -128, 127);
                    break;
                case T_TEXHITSCAN:
                    flags |= PICANM_TEXHITSCAN_BIT;
                    break;
                case T_NOFULLBRIGHT:
                    flags |= PICANM_NOFULLBRIGHT_BIT;
                    break;
                case T_TEXTURE:
                    istexture = 1;
                    break;
                default:
                    break;
                }
            }

            if (EDUKE32_PREDICT_FALSE((unsigned)tile >= MAXUSERTILES))
            {
                initprintf("Error: missing or invalid 'tile number' for texture definition near line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,texturetokptr));
                break;
            }

            if (!fn)
            {
                // tilefromtexture <tile> { texhitscan }  sets the bit but doesn't change tile data
                picanm[tile].sf |= flags;
                if (havexoffset)
                    picanm[tile].xofs = xoffset;
                if (haveyoffset)
                    picanm[tile].yofs = yoffset;

                if (EDUKE32_PREDICT_FALSE(flags == 0 && !havexoffset && !haveyoffset))
                    initprintf("\nError: missing 'file name' for tilefromtexture definition near line %s:%d",
                               script->filename, scriptfile_getlinum(script,texturetokptr));
                break;
            }

            int32_t const texstatus = Defs_ImportTileFromTexture(fn, tile, alphacut, istexture);
            if (texstatus == -3)
                initprintf("Error: No palette loaded, in tilefromtexture definition near line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,texturetokptr));
            if (texstatus == -(3<<8))
                initprintf("Error: \"%s\" has more than one tile, in tilefromtexture definition near line %s:%d\n",
                           fn, script->filename, scriptfile_getlinum(script,texturetokptr));
            if (texstatus < 0)
                break;

            picanm[tile].sf |= flags;

            if (havexoffset)
                picanm[tile].xofs = xoffset;
            else if (texstatus == 0)
                picanm[tile].xofs = 0;

            if (haveyoffset)
                picanm[tile].yofs = yoffset;
            else if (texstatus == 0)
                picanm[tile].yofs = 0;
        }
        break;
        case T_COPYTILE:
        {
            char *blockend;
            int32_t tile = -1, source;
            int32_t havetile = 0, havexoffset = 0, haveyoffset = 0;
            int32_t xoffset = 0, yoffset = 0;
            int32_t flags = 0;
            int32_t tsiz = 0;

            static const tokenlist copytiletokens[] =
            {
                { "tile",            T_TILE },
                { "pal",             T_PAL },
                { "xoffset",         T_XOFFSET },
                { "xoff",            T_XOFFSET },
                { "yoffset",         T_YOFFSET },
                { "yoff",            T_YOFFSET },
                { "texhitscan",      T_TEXHITSCAN },
                { "nofullbright",    T_NOFULLBRIGHT },
            };

            if (scriptfile_getsymbol(script,&tile)) break;
            source = tile; // without a "tile" token, we still palettize self by default
            if (scriptfile_getbraces(script,&blockend)) break;
            while (script->textptr < blockend)
            {
                int32_t token = getatoken(script,copytiletokens,ARRAY_SIZE(copytiletokens));
                switch (token)
                {
                case T_TILE:
                {
                    int32_t tempsource;
                    scriptfile_getsymbol(script,&tempsource);

                    if (check_tile("copytile", tempsource, script, cmdtokptr))
                        break;
                    if ((tsiz = Defs_LoadTileIntoBuffer(tempsource)) <= 0)
                        break;
                    source = tempsource;

                    havetile = 1;
                    break;
                }
                case T_PAL:
                {
                    int32_t temppal;
                    scriptfile_getsymbol(script,&temppal);

                    // palettize self case
                    if (!havetile)
                    {
                        if (check_tile("copytile", source, script, cmdtokptr))
                            break;
                        if ((tsiz = Defs_LoadTileIntoBuffer(source)) <= 0)
                            break;
                        havetile = 1;
                    }

                    if (EDUKE32_PREDICT_FALSE((unsigned)temppal >= MAXPALOOKUPS-RESERVEDPALS))
                    {
                        initprintf("Error: copytile 'palette number' out of range (max=%d)\n",
                                   MAXPALOOKUPS-RESERVEDPALS-1);
                        break;
                    }

                    Defs_ApplyPaletteToTileBuffer(tsiz, temppal);
                    break;
                }
                case T_XOFFSET:
                    havexoffset = 1;
                    scriptfile_getsymbol(script,&xoffset); break;
                case T_YOFFSET:
                    haveyoffset = 1;
                    scriptfile_getsymbol(script,&yoffset); break;
                case T_TEXHITSCAN:
                    flags |= PICANM_TEXHITSCAN_BIT;
                    break;
                case T_NOFULLBRIGHT:
                    flags |= PICANM_NOFULLBRIGHT_BIT;
                    break;
                default:
                    break;
                }
            }

            if (check_tile("copytile", tile, script, cmdtokptr))
                break;

            if (havetile)
            {
                E_CreateFakeTile(tile, tsiz, faketilebuffer);
            }
            else // if !havetile, we have never confirmed a valid source
            {
                if (check_tile("copytile", source, script, cmdtokptr))
                    break;
            }

            if (tsiz <= 0)
            {
                E_UndefineTile(tile);
                break;
            }

            set_tilesiz(tile, tilesiz[source].x, tilesiz[source].y);
            picanm[tile].xofs = havexoffset ? clamp(xoffset, -128, 127) : picanm[source].xofs;
            picanm[tile].yofs = haveyoffset ? clamp(yoffset, -128, 127) : picanm[source].yofs;
            picanm[tile].sf = (picanm[tile].sf & ~PICANM_MISC_MASK) | (picanm[source].sf & PICANM_MISC_MASK) | flags;

        }
        break;
        case T_IMPORTTILE:
        {
            int32_t tile;
            char *fn;

            if (scriptfile_getsymbol(script,&tile)) break;
            if (scriptfile_getstring(script,&fn))  break;

            if (check_tile("importtile", tile, script, cmdtokptr))
                break;

            int32_t const texstatus = Defs_ImportTileFromTexture(fn, tile, 255, 0);
            if (texstatus == -3)
                initprintf("Error: No palette loaded, in importtile definition near line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
            if (texstatus == -(3<<8))
                initprintf("Error: \"%s\" has more than one tile, in importtile definition near line %s:%d\n",
                           fn, script->filename, scriptfile_getlinum(script,cmdtokptr));
            if (texstatus < 0)
                break;

            Bmemset(&picanm[tile], 0, sizeof(picanm_t));

            break;
        }
        case T_DUMMYTILE:
        {
            int32_t tile, xsiz, ysiz;

            if (scriptfile_getsymbol(script,&tile)) break;
            if (scriptfile_getsymbol(script,&xsiz)) break;
            if (scriptfile_getsymbol(script,&ysiz)) break;

            if (check_tile("dummytile", tile, script, cmdtokptr))
                break;

            if ((int16_t) xsiz == 0 || (int16_t) ysiz == 0)
            {
                E_UndefineTile(tile);
                break;
            }

            if (xsiz > 0 && ysiz > 0)
            {
                set_tilesiz(tile, xsiz, ysiz);
                Bmemset(&picanm[tile], 0, sizeof(picanm_t));
                E_CreateDummyTile(tile);
            }

            break;
        }
        case T_DUMMYTILERANGE:
        {
            int32_t tile1,tile2,xsiz,ysiz,i;

            if (scriptfile_getnumber(script,&tile1)) break;
            if (scriptfile_getnumber(script,&tile2)) break;
            if (scriptfile_getnumber(script,&xsiz)) break;
            if (scriptfile_getnumber(script,&ysiz)) break;

            if (check_tile_range("dummytilerange", &tile1, &tile2, script, cmdtokptr))
                break;

            if (xsiz < 0 || ysiz < 0)
                break;  // TODO: message

            if ((int16_t) xsiz == 0 || (int16_t) ysiz == 0)
            {
                for (i=tile1; i<=tile2; i++)
                    E_UndefineTile(i);
                break;
            }

            for (i=tile1; i<=tile2; i++)
            {
                set_tilesiz(i, xsiz, ysiz);
                Bmemset(&picanm[i], 0, sizeof(picanm_t));
                E_CreateDummyTile(i);
            }

            break;
        }

        case T_UNDEFINETILE:
        {
            int32_t tile;

            if (scriptfile_getsymbol(script,&tile)) break;

            if (check_tile("undefinetile", tile, script, cmdtokptr))
                break;

            E_UndefineTile(tile);

            break;
        }
        case T_UNDEFINETILERANGE:
        {
            int32_t tile1, tile2;

            if (scriptfile_getnumber(script,&tile1)) break;
            if (scriptfile_getnumber(script,&tile2)) break;

            if (check_tile_range("undefinetilerange", &tile1, &tile2, script, cmdtokptr))
                break;

            for (bssize_t i = tile1; i <= tile2; i++)
                E_UndefineTile(i);

            break;
        }

        case T_DEFINEMODEL:
        {
            char *modelfn;
            double scale;
            int32_t shadeoffs;

            if (scriptfile_getstring(script,&modelfn)) break;
            if (scriptfile_getdouble(script,&scale)) break;
            if (scriptfile_getnumber(script,&shadeoffs)) break;

#ifdef USE_OPENGL
            lastmodelid = md_loadmodel(modelfn);
            if (EDUKE32_PREDICT_FALSE(lastmodelid < 0))
            {
                initprintf("Warning: Failed loading MD2/MD3 model \"%s\"\n", modelfn);
                break;
            }
            md_setmisc(lastmodelid,(float)scale, shadeoffs,0.0,0.0,0);
# ifdef POLYMER
            if (glrendmode == REND_POLYMER)
                md3postload_polymer((md3model_t *)models[lastmodelid]);
# endif
#endif
            modelskin = lastmodelskin = 0;
            seenframe = 0;
        }
        break;
        case T_DEFINEMODELFRAME:
        {
            char *framename;
#ifdef USE_OPENGL
            char happy=1;
            int32_t tilex;
#endif
            int32_t ftilenume, ltilenume;

            if (scriptfile_getstring(script,&framename)) break;
            if (scriptfile_getnumber(script,&ftilenume)) break; //first tile number
            if (scriptfile_getnumber(script,&ltilenume)) break; //last tile number (inclusive)

            if (check_tile_range("definemodelframe", &ftilenume, &ltilenume, script, cmdtokptr))
                break;

            if (EDUKE32_PREDICT_FALSE(lastmodelid < 0))
            {
#ifdef USE_OPENGL
                initprintf("Warning: Ignoring frame definition.\n");
#endif
                break;
            }
#ifdef USE_OPENGL
            for (tilex = ftilenume; tilex <= ltilenume && happy; tilex++)
            {
                switch (md_defineframe(lastmodelid, framename, tilex, max(0,modelskin), 0.0f,0))
                {
                case -1:
                    happy = 0; break; // invalid model id!?
                case -2:
                    initprintf("Invalid tile number on line %s:%d\n",
                               script->filename, scriptfile_getlinum(script,cmdtokptr));
                    happy = 0;
                    break;
                case -3:
                    initprintf("Invalid frame name on line %s:%d\n",
                               script->filename, scriptfile_getlinum(script,cmdtokptr));
                    happy = 0;
                    break;
                default:
                    break;
                }
            }
#endif
            seenframe = 1;
        }
        break;
        case T_DEFINEMODELANIM:
        {
            char *startframe, *endframe;
            int32_t flags;
            double dfps;

            if (scriptfile_getstring(script,&startframe)) break;
            if (scriptfile_getstring(script,&endframe)) break;
            if (scriptfile_getdouble(script,&dfps)) break; //animation frame rate
            if (scriptfile_getnumber(script,&flags)) break;

            if (EDUKE32_PREDICT_FALSE(lastmodelid < 0))
            {
#ifdef USE_OPENGL
                initprintf("Warning: Ignoring animation definition.\n");
#endif
                break;
            }
#ifdef USE_OPENGL
            switch (md_defineanimation(lastmodelid, startframe, endframe, (int32_t)(dfps*(65536.0*.001)), flags))
            {
            case 0:
                break;
            case -1:
                break; // invalid model id!?
            case -2:
                initprintf("Invalid starting frame name on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            case -3:
                initprintf("Invalid ending frame name on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            case -4:
                initprintf("Out of memory on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }
#endif
        }
        break;
        case T_DEFINEMODELSKIN:
        {
            int32_t palnum;
            char *skinfn;

            if (scriptfile_getsymbol(script,&palnum)) break;
            if (scriptfile_getstring(script,&skinfn)) break; //skin filename

            // if we see a sequence of definemodelskin, then a sequence of definemodelframe,
            // and then a definemodelskin, we need to increment the skin counter.
            //
            // definemodel "mymodel.md2" 1 1
            // definemodelskin 0 "normal.png"   // skin 0
            // definemodelskin 21 "normal21.png"
            // definemodelframe "foo" 1000 1002   // these use skin 0
            // definemodelskin 0 "wounded.png"   // skin 1
            // definemodelskin 21 "wounded21.png"
            // definemodelframe "foo2" 1003 1004   // these use skin 1
            // selectmodelskin 0         // resets to skin 0
            // definemodelframe "foo3" 1005 1006   // these use skin 0
            if (seenframe) { modelskin = ++lastmodelskin; }
            seenframe = 0;

            if (check_file_exist(skinfn))
                break;

#ifdef USE_OPENGL
            switch (md_defineskin(lastmodelid, skinfn, palnum, max(0,modelskin), 0, 0.0f, 1.0f, 1.0f, 0))
            {
            case 0:
                break;
            case -1:
                break; // invalid model id!?
            case -2:
                initprintf("Invalid skin filename on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            case -3:
                initprintf("Invalid palette number on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            case -4:
                initprintf("Out of memory on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }
#endif
        }
        break;
        case T_SELECTMODELSKIN:
        {
            if (scriptfile_getsymbol(script,&modelskin)) break;
        }
        break;
        case T_DEFINEVOXEL:
        {
            char *fn;

            if (EDUKE32_PREDICT_FALSE(scriptfile_getstring(script,&fn)))
                break; //voxel filename

            if (EDUKE32_PREDICT_FALSE(nextvoxid == MAXVOXELS))
            {
                initprintf("Maximum number of voxels (%d) already defined.\n", MAXVOXELS);
                break;
            }

            if (EDUKE32_PREDICT_FALSE(qloadkvx(nextvoxid, fn)))
            {
                initprintf("Failure loading voxel file \"%s\"\n",fn);
                break;
            }

            lastvoxid = nextvoxid++;
        }
        break;
        case T_DEFINEVOXELTILES:
        {
            int32_t ftilenume, ltilenume, tilex;

            if (scriptfile_getnumber(script,&ftilenume)) break; //1st tile #
            if (scriptfile_getnumber(script,&ltilenume)) break; //last tile #

            if (check_tile_range("definevoxeltiles", &ftilenume, &ltilenume, script, cmdtokptr))
                break;

            if (EDUKE32_PREDICT_FALSE(lastvoxid < 0))
            {
                initprintf("Warning: Ignoring voxel tiles definition.\n");
                break;
            }

            for (tilex = ftilenume; tilex <= ltilenume; tilex++)
                tiletovox[tilex] = lastvoxid;
        }
        break;

        // NEW (ENCOURAGED) DEFINITION SYNTAX
        case T_MODEL:
        {
            char *modelend, *modelfn;
            double scale=1.0, mzadd=0.0, myoffset=0.0;
            int32_t shadeoffs=0, pal=0, flags=0;
            uint8_t usedframebitmap[1024>>3];

            int32_t model_ok = 1;

            static const tokenlist modeltokens[] =
            {
                { "scale",    T_SCALE    },
                { "shade",    T_SHADE    },
                { "zadd",     T_ZADD     },
                { "yoffset",  T_YOFFSET  },
                { "frame",    T_FRAME    },
                { "anim",     T_ANIM     },
                { "skin",     T_SKIN     },
                { "detail",   T_DETAIL   },
                { "glow",     T_GLOW     },
                { "specular", T_SPECULAR },
                { "normal",   T_NORMAL   },
                { "hud",      T_HUD      },
                { "flags",    T_FLAGS    },
            };

            Bmemset(usedframebitmap, 0, sizeof(usedframebitmap));

            modelskin = lastmodelskin = 0;
            seenframe = 0;

            if (scriptfile_getstring(script,&modelfn)) break;
            if (scriptfile_getbraces(script,&modelend)) break;
#ifdef USE_OPENGL
            lastmodelid = md_loadmodel(modelfn);
            if (EDUKE32_PREDICT_FALSE(lastmodelid < 0))
            {
                initprintf("Warning: Failed loading MD2/MD3 model \"%s\"\n", modelfn);
                script->textptr = modelend+1;
                break;
            }
#endif
            while (script->textptr < modelend)
            {
                int32_t token = getatoken(script,modeltokens,ARRAY_SIZE(modeltokens));
                switch (token)
                {
                    //case T_ERROR: initprintf("Error on line %s:%d in model tokens\n", script->filename,script->linenum); break;
                case T_SCALE:
                    scriptfile_getdouble(script,&scale); break;
                case T_SHADE:
                    scriptfile_getnumber(script,&shadeoffs); break;
                case T_ZADD:
                    scriptfile_getdouble(script,&mzadd); break;
                case T_YOFFSET:
                    scriptfile_getdouble(script,&myoffset); break;
                case T_FLAGS:
                    scriptfile_getnumber(script,&flags); break;
                case T_FRAME:
                {
                    char *frametokptr = script->ltextptr;
                    char *frameend, *framename = 0;
#ifdef USE_OPENGL
                    char happy=1;
                    int32_t tilex = 0, framei;
#endif
                    int32_t ftilenume = -1, ltilenume = -1;
                    double smoothduration = 0.1f;

                    static const tokenlist modelframetokens[] =
                    {
                        { "pal",              T_PAL               },
                        { "frame",            T_FRAME             },
                        { "name",             T_FRAME             },
                        { "tile",             T_TILE              },
                        { "tile0",            T_TILE0             },
                        { "tile1",            T_TILE1             },
                        { "smoothduration",   T_SMOOTHDURATION    },
                    };

                    if (scriptfile_getbraces(script,&frameend)) break;
                    while (script->textptr < frameend)
                    {
                        switch (getatoken(script,modelframetokens,ARRAY_SIZE(modelframetokens)))
                        {
                        case T_PAL:
                            scriptfile_getnumber(script,&pal); break;
                        case T_FRAME:
                            scriptfile_getstring(script,&framename); break;
                        case T_TILE:
                            scriptfile_getsymbol(script,&ftilenume); ltilenume = ftilenume; break;
                        case T_TILE0:
                            scriptfile_getsymbol(script,&ftilenume); break; //first tile number
                        case T_TILE1:
                            scriptfile_getsymbol(script,&ltilenume); break; //last tile number (inclusive)
                        case T_SMOOTHDURATION:
                            scriptfile_getdouble(script,&smoothduration); break;
                        }
                    }

                    if (check_tile_range("model: frame", &ftilenume, &ltilenume, script, frametokptr))
                    {
                        model_ok = 0;
                        break;
                    }

                    if (EDUKE32_PREDICT_FALSE(lastmodelid < 0))
                    {
#ifdef USE_OPENGL
                        initprintf("Warning: ignoring frame definition on line %s:%d.\n",
                                   script->filename, scriptfile_getlinum(script,frametokptr));
#endif
                        break;
                    }

                    if (smoothduration > 1.0)
                    {
                        initprintf("Warning: smoothduration out of range on line %s:%d.\n",
                                   script->filename, scriptfile_getlinum(script,frametokptr));
                        smoothduration = 1.0;
                    }
#ifdef USE_OPENGL
                    for (tilex = ftilenume; tilex <= ltilenume && happy; tilex++)
                    {
                        framei = md_defineframe(lastmodelid, framename, tilex, max(0,modelskin), smoothduration,pal);
                        switch (framei)
                        {
                        case -1:
                            happy = 0; break; // invalid model id!?
                        case -2:
                            initprintf("Invalid tile number on line %s:%d\n",
                                       script->filename, scriptfile_getlinum(script,frametokptr));
                            happy = 0;
                            break;
                        case -3:
                            initprintf("Invalid frame name on line %s:%d\n",
                                       script->filename, scriptfile_getlinum(script,frametokptr));
                            happy = 0;
                            break;
                        default:
                            if (framei >= 0 && framei<1024)
                                usedframebitmap[framei>>3] |= (1<<(framei&7));
                        }

                        model_ok &= happy;
                    }
#endif
                    seenframe = 1;
                }
                break;
                case T_ANIM:
                {
                    char *animtokptr = script->ltextptr;
                    char *animend, *startframe = 0, *endframe = 0, happy=1;
                    int32_t flags = 0;
                    double dfps = 1.0;

                    static const tokenlist modelanimtokens[] =
                    {
                        { "frame0", T_FRAME0 },
                        { "frame1", T_FRAME1 },
                        { "fps",    T_FPS    },
                        { "flags",  T_FLAGS  },
                    };

                    if (scriptfile_getbraces(script,&animend)) break;
                    while (script->textptr < animend)
                    {
                        switch (getatoken(script,modelanimtokens,ARRAY_SIZE(modelanimtokens)))
                        {
                        case T_FRAME0:
                            scriptfile_getstring(script,&startframe); break;
                        case T_FRAME1:
                            scriptfile_getstring(script,&endframe); break;
                        case T_FPS:
                            scriptfile_getdouble(script,&dfps); break; //animation frame rate
                        case T_FLAGS:
                            scriptfile_getsymbol(script,&flags); break;
                        }
                    }

                    if (EDUKE32_PREDICT_FALSE(!startframe)) initprintf("Error: missing 'start frame' for anim definition near line %s:%d\n", script->filename, scriptfile_getlinum(script,animtokptr)), happy = 0;
                    if (EDUKE32_PREDICT_FALSE(!endframe)) initprintf("Error: missing 'end frame' for anim definition near line %s:%d\n", script->filename, scriptfile_getlinum(script,animtokptr)), happy = 0;
                    model_ok &= happy;
                    if (EDUKE32_PREDICT_FALSE(!happy)) break;

                    if (EDUKE32_PREDICT_FALSE(lastmodelid < 0))
                    {
#ifdef USE_OPENGL
                        initprintf("Warning: Ignoring animation definition.\n");
#endif
                        break;
                    }
#ifdef USE_OPENGL
                    switch (md_defineanimation(lastmodelid, startframe, endframe, (int32_t)(dfps*(65536.0*.001)), flags))
                    {
                    case 0:
                        break;
                    case -1:
                        break; // invalid model id!?
                    case -2:
                        initprintf("Invalid starting frame name on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,animtokptr));
                        model_ok = 0;
                        break;
                    case -3:
                        initprintf("Invalid ending frame name on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,animtokptr));
                        model_ok = 0;
                        break;
                    case -4:
                        initprintf("Out of memory on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,animtokptr));
                        model_ok = 0;
                        break;
                    }
#endif
                }
                break;
                case T_SKIN: case T_DETAIL: case T_GLOW: case T_SPECULAR: case T_NORMAL:
                {
                    char *skintokptr = script->ltextptr;
                    char *skinend, *skinfn = 0;
                    int32_t palnum = 0, surfnum = 0;
                    double param = 1.0, specpower = 1.0, specfactor = 1.0;
#ifdef USE_OPENGL
                    int32_t flags = 0;
#endif

                    static const tokenlist modelskintokens[] =
                    {
                        { "pal",           T_PAL        },
                        { "file",          T_FILE       },
                        { "surf",          T_SURF       },
                        { "surface",       T_SURF       },
                        { "intensity",     T_PARAM      },
                        { "scale",         T_PARAM      },
                        { "detailscale",   T_PARAM      },
                        { "specpower",     T_SPECPOWER  }, { "specularpower",  T_SPECPOWER  }, { "parallaxscale", T_SPECPOWER },
                        { "specfactor",    T_SPECFACTOR }, { "specularfactor", T_SPECFACTOR }, { "parallaxbias", T_SPECFACTOR },
                        { "nocompress",    T_NOCOMPRESS },
                        { "nodownsize",    T_NODOWNSIZE },
                        { "forcefilter",  T_FORCEFILTER },
                        { "artquality",    T_ARTQUALITY },
                    };

                    if (scriptfile_getbraces(script,&skinend)) break;
                    while (script->textptr < skinend)
                    {
                        switch (getatoken(script,modelskintokens,ARRAY_SIZE(modelskintokens)))
                        {
                        case T_PAL:
                            scriptfile_getsymbol(script,&palnum); break;
                        case T_PARAM:
                            scriptfile_getdouble(script,&param); break;
                        case T_SPECPOWER:
                            scriptfile_getdouble(script,&specpower); break;
                        case T_SPECFACTOR:
                            scriptfile_getdouble(script,&specfactor); break;
                        case T_FILE:
                            scriptfile_getstring(script,&skinfn); break; //skin filename
                        case T_SURF:
                            scriptfile_getnumber(script,&surfnum); break;
#ifdef USE_OPENGL
                        case T_NOCOMPRESS:
                            flags |= HICR_NOTEXCOMPRESS; break;
                        case T_NODOWNSIZE:
                            flags |= HICR_NODOWNSIZE; break;
                        case T_FORCEFILTER:
                            flags |= HICR_FORCEFILTER; break;
                        case T_ARTQUALITY:
                            flags |= HICR_ARTIMMUNITY; break;
#endif
                        }
                    }

                    if (EDUKE32_PREDICT_FALSE(!skinfn))
                    {
                        initprintf("Error: missing 'skin filename' for skin definition near line %s:%d\n", script->filename, scriptfile_getlinum(script,skintokptr));
                        model_ok = 0;
                        break;
                    }

                    if (seenframe) { modelskin = ++lastmodelskin; }
                    seenframe = 0;

                    switch (token)
                    {
                    case T_DETAIL:
                        palnum = DETAILPAL;
                        param = 1.0f / param;
                        break;
                    case T_GLOW:
                        palnum = GLOWPAL;
                        break;
                    case T_SPECULAR:
                        palnum = SPECULARPAL;
                        break;
                    case T_NORMAL:
                        palnum = NORMALPAL;
                        break;
                    }

                    if (check_file_exist(skinfn))
                        break;

#ifdef USE_OPENGL
                    switch (md_defineskin(lastmodelid, skinfn, palnum, max(0,modelskin), surfnum, param, specpower, specfactor, flags))
                    {
                    case 0:
                        break;
                    case -1:
                        break; // invalid model id!?
                    case -2:
                        initprintf("Invalid skin filename on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,skintokptr));
                        model_ok = 0;
                        break;
                    case -3:
                        initprintf("Invalid palette number on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,skintokptr));
                        model_ok = 0;
                        break;
                    case -4:
                        initprintf("Out of memory on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,skintokptr));
                        model_ok = 0;
                        break;
                    }
#endif
                }
                break;
                case T_HUD:
                {
                    char *hudtokptr = script->ltextptr;
                    char *frameend;
#ifdef USE_OPENGL
                    char happy=1;
                    int32_t tilex = 0;
#endif
                    int32_t ftilenume = -1, ltilenume = -1, flags = 0, fov = -1, angadd = 0;
                    double xadd = 0.0, yadd = 0.0, zadd = 0.0;

                    static const tokenlist modelhudtokens[] =
                    {
                        { "tile",   T_TILE   },
                        { "tile0",  T_TILE0  },
                        { "tile1",  T_TILE1  },
                        { "xadd",   T_XADD   },
                        { "yadd",   T_YADD   },
                        { "zadd",   T_ZADD   },
                        { "angadd", T_ANGADD },
                        { "fov",    T_FOV    },
                        { "hide",   T_HIDE   },
                        { "nobob",  T_NOBOB  },
                        { "flipped",T_FLIPPED},
                        { "nodepth",T_NODEPTH},
                    };

                    if (scriptfile_getbraces(script,&frameend)) break;
                    while (script->textptr < frameend)
                    {
                        switch (getatoken(script,modelhudtokens,ARRAY_SIZE(modelhudtokens)))
                        {
                        case T_TILE:
                            scriptfile_getsymbol(script,&ftilenume); ltilenume = ftilenume; break;
                        case T_TILE0:
                            scriptfile_getsymbol(script,&ftilenume); break; //first tile number
                        case T_TILE1:
                            scriptfile_getsymbol(script,&ltilenume); break; //last tile number (inclusive)
                        case T_XADD:
                            scriptfile_getdouble(script,&xadd); break;
                        case T_YADD:
                            scriptfile_getdouble(script,&yadd); break;
                        case T_ZADD:
                            scriptfile_getdouble(script,&zadd); break;
                        case T_ANGADD:
                            scriptfile_getsymbol(script,&angadd); break;
                        case T_FOV:
                            scriptfile_getsymbol(script,&fov); break;
                        case T_HIDE:
                            flags |= HUDFLAG_HIDE; break;
                        case T_NOBOB:
                            flags |= HUDFLAG_NOBOB; break;
                        case T_FLIPPED:
                            flags |= HUDFLAG_FLIPPED; break;
                        case T_NODEPTH:
                            flags |= HUDFLAG_NODEPTH; break;
                        }
                    }

                    if (EDUKE32_PREDICT_FALSE(check_tile_range("hud", &ftilenume, &ltilenume, script, hudtokptr)))
                    {
                        model_ok = 0;
                        break;
                    }

                    if (EDUKE32_PREDICT_FALSE(lastmodelid < 0))
                    {
#ifdef USE_OPENGL
                        initprintf("Warning: Ignoring frame definition.\n");
#endif
                        break;
                    }
#ifdef USE_OPENGL
                    for (tilex = ftilenume; tilex <= ltilenume && happy; tilex++)
                    {
                        vec3f_t const add = { (float)xadd, (float)yadd, (float)zadd };
                        switch (md_definehud(lastmodelid, tilex, add, angadd, flags, fov))
                        {
                        case 0:
                            break;
                        case -1:
                            happy = 0; break; // invalid model id!?
                        case -2:
                            initprintf("Invalid tile number on line %s:%d\n",
                                       script->filename, scriptfile_getlinum(script,hudtokptr));
                            happy = 0;
                            break;
                        case -3:
                            initprintf("Invalid frame name on line %s:%d\n",
                                       script->filename, scriptfile_getlinum(script,hudtokptr));
                            happy = 0;
                            break;
                        }

                        model_ok &= happy;
                    }
#endif
                }
                break;
                }
            }

#ifdef USE_OPENGL
            if (EDUKE32_PREDICT_FALSE(!model_ok))
            {
                if (lastmodelid >= 0)
                {
                    initprintf("Removing model %d due to errors.\n", lastmodelid);
                    md_undefinemodel(lastmodelid);
                    nextmodelid--;
                }
                break;
            }

            md_setmisc(lastmodelid,(float)scale,shadeoffs,(float)mzadd,(float)myoffset,flags);

            // thin out the loaded model by throwing away unused frames
            // FIXME: CURRENTLY DISABLED: interpolation may access frames we consider 'unused'?
# if 0
            if (models[lastmodelid]->mdnum==3 && ((md3model_t *)models[lastmodelid])->numframes <= 1024)
            {
#  ifdef DEBUG_MODEL_MEM
                md3model_t *m = (md3model_t *)models[lastmodelid];
                int32_t i, onumframes;
                onumframes = m->numframes;
                i =
#  endif
                md_thinoutmodel(lastmodelid, usedframebitmap);
#  ifdef DEBUG_MODEL_MEM
                if (i>=0 && i<onumframes)
                    initprintf("used %d/%d frames: %s\n", i, onumframes, modelfn);
                else if (i<0)
                    initprintf("md_thinoutmodel returned %d: %s\n", i, modelfn);
#  endif
            }
# endif

            if (glrendmode == REND_POLYMER)
                md3postload_polymer((md3model_t *)models[lastmodelid]);
#endif

            modelskin = lastmodelskin = 0;
            seenframe = 0;

        }
        break;
        case T_VOXEL:
        {
            char *voxeltokptr = script->ltextptr;
            char *fn, *modelend;
            int32_t tile0 = MAXTILES, tile1 = -1, tilex = -1;

            static const tokenlist voxeltokens[] =
            {
                { "tile",   T_TILE   },
                { "tile0",  T_TILE0  },
                { "tile1",  T_TILE1  },
                { "scale",  T_SCALE  },
            };

            if (EDUKE32_PREDICT_FALSE(scriptfile_getstring(script,&fn)))
                break; //voxel filename

            if (EDUKE32_PREDICT_FALSE(nextvoxid == MAXVOXELS))
            {
                initprintf("Maximum number of voxels (%d) already defined.\n", MAXVOXELS);
                break;
            }

            if (EDUKE32_PREDICT_FALSE(qloadkvx(nextvoxid, fn)))
            {
                initprintf("Failure loading voxel file \"%s\"\n",fn);
                break;
            }

            lastvoxid = nextvoxid++;

            if (scriptfile_getbraces(script,&modelend)) break;
            while (script->textptr < modelend)
            {
                switch (getatoken(script, voxeltokens, ARRAY_SIZE(voxeltokens)))
                {
                    //case T_ERROR: initprintf("Error on line %s:%d in voxel tokens\n", script->filename,linenum); break;
                case T_TILE:
                    scriptfile_getsymbol(script,&tilex);

                    if (check_tile("voxel", tilex, script, voxeltokptr))
                        break;

                    tiletovox[tilex] = lastvoxid;
                    break;

                case T_TILE0:
                    scriptfile_getsymbol(script,&tile0);
                    break; //1st tile #

                case T_TILE1:
                    scriptfile_getsymbol(script,&tile1);

                    if (check_tile_range("voxel", &tile0, &tile1, script, voxeltokptr))
                        break;

                    for (tilex=tile0; tilex<=tile1; tilex++)
                        tiletovox[tilex] = lastvoxid;
                    break; //last tile number (inclusive)

                case T_SCALE:
                {
                    double scale=1.0;
                    scriptfile_getdouble(script,&scale);
                    voxscale[lastvoxid] = (int32_t)(65536*scale);
#ifdef USE_OPENGL
                    if (voxmodels[lastvoxid])
                        voxmodels[lastvoxid]->scale = scale;
#endif
                    break;
                }
                }
            }
            lastvoxid = -1;
        }
        break;
        case T_SKYBOX:
        {
            char *skyboxtokptr = script->ltextptr;
            char *fn[6] = {0,0,0,0,0,0};
            char *modelend;
            int32_t i, tile = -1, pal = 0, happy = 1;
#ifdef USE_OPENGL
            int32_t flags = 0;
#endif

            static const tokenlist skyboxtokens[] =
            {
                { "tile"   ,T_TILE   },
                { "pal"    ,T_PAL    },
                { "ft"     ,T_FRONT  },{ "front"  ,T_FRONT  },{ "forward",T_FRONT  },
                { "rt"     ,T_RIGHT  },{ "right"  ,T_RIGHT  },
                { "bk"     ,T_BACK   },{ "back"   ,T_BACK   },
                { "lf"     ,T_LEFT   },{ "left"   ,T_LEFT   },{ "lt"     ,T_LEFT   },
                { "up"     ,T_TOP    },{ "top"    ,T_TOP    },{ "ceiling",T_TOP    },{ "ceil"   ,T_TOP    },
                { "dn"     ,T_BOTTOM },{ "bottom" ,T_BOTTOM },{ "floor"  ,T_BOTTOM },{ "down"   ,T_BOTTOM },
                { "nocompress", T_NOCOMPRESS },
                { "nodownsize", T_NODOWNSIZE },
                { "forcefilter", T_FORCEFILTER },
                { "artquality", T_ARTQUALITY },
            };

            if (scriptfile_getbraces(script,&modelend)) break;
            while (script->textptr < modelend)
            {
                switch (getatoken(script,skyboxtokens,ARRAY_SIZE(skyboxtokens)))
                {
                    //case T_ERROR: initprintf("Error on line %s:%d in skybox tokens\n",script->filename,linenum); break;
                case T_TILE:
                    scriptfile_getsymbol(script,&tile); break;
                case T_PAL:
                    scriptfile_getsymbol(script,&pal); break;
                case T_FRONT:
                    scriptfile_getstring(script,&fn[0]); break;
                case T_RIGHT:
                    scriptfile_getstring(script,&fn[1]); break;
                case T_BACK:
                    scriptfile_getstring(script,&fn[2]); break;
                case T_LEFT:
                    scriptfile_getstring(script,&fn[3]); break;
                case T_TOP:
                    scriptfile_getstring(script,&fn[4]); break;
                case T_BOTTOM:
                    scriptfile_getstring(script,&fn[5]); break;
#ifdef USE_OPENGL
                case T_NOCOMPRESS:
                    flags |= HICR_NOTEXCOMPRESS; break;
                case T_NODOWNSIZE:
                    flags |= HICR_NODOWNSIZE; break;
                case T_FORCEFILTER:
                    flags |= HICR_FORCEFILTER; break;
                case T_ARTQUALITY:
                    flags |= HICR_ARTIMMUNITY; break;
#endif
                }
            }

            if (EDUKE32_PREDICT_FALSE(tile < 0)) initprintf("Error: skybox: missing 'tile number' near line %s:%d\n", script->filename, scriptfile_getlinum(script,skyboxtokptr)), happy=0;
            for (i=0; i<6; i++)
            {
                if (EDUKE32_PREDICT_FALSE(!fn[i])) initprintf("Error: skybox: missing '%s filename' near line %s:%d\n", skyfaces[i], script->filename, scriptfile_getlinum(script,skyboxtokptr)), happy = 0;
                // FIXME?
                if (check_file_exist(fn[i]))
                    happy = 0;
            }
            if (!happy) break;

#ifdef USE_OPENGL
            hicsetskybox(tile,pal,fn, flags);
#endif
        }
        break;
        case T_HIGHPALOOKUP:
        {
            char *highpaltokptr = script->ltextptr;
            int32_t basepal=-1, pal=-1;
            char *fn = NULL;
            char *highpalend;
#ifdef POLYMER
            int32_t fd;
            char *highpaldata;
#endif
            static const tokenlist highpaltokens[] =
            {
                { "basepal",   T_BASEPAL },
                { "pal",   T_PAL },
                { "file",  T_FILE }
            };

            if (scriptfile_getbraces(script,&highpalend)) break;
            while (script->textptr < highpalend)
            {
                switch (getatoken(script,highpaltokens,ARRAY_SIZE(highpaltokens)))
                {
                case T_BASEPAL:
                    scriptfile_getsymbol(script,&basepal);   break;
                case T_PAL:
                    scriptfile_getsymbol(script,&pal);   break;
                case T_FILE:
                    scriptfile_getstring(script,&fn); break;
                }
            }
            if (EDUKE32_PREDICT_FALSE((unsigned)basepal >= MAXBASEPALS))
            {
                initprintf("Error: missing or invalid 'base palette number' for highpalookup definition "
                           "near line %s:%d\n", script->filename, scriptfile_getlinum(script,highpaltokptr));
                break;
            }

            if (EDUKE32_PREDICT_FALSE((unsigned)pal >= MAXPALOOKUPS - RESERVEDPALS))
            {
                initprintf("Error: missing or invalid 'palette number' for highpalookup definition near "
                           "line %s:%d\n", script->filename, scriptfile_getlinum(script,highpaltokptr));
                break;
            }

            if (EDUKE32_PREDICT_FALSE(!fn))
            {
                initprintf("Error: missing 'file name' for highpalookup definition near line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,highpaltokptr));
                break;
            }

            if (check_file_exist(fn))
                break;

#ifdef POLYMER
            fd = kopen4load(fn, 0);

            // load the highpalookup and send it to polymer
            highpaldata = (char *)Xmalloc(PR_HIGHPALOOKUP_DATA_SIZE);

            {
                char *filebuf;
                int32_t xsiz, ysiz, filesize, i;

                filesize = kfilelength(fd);

                filebuf = (char *)Xmalloc(filesize);

                klseek(fd, 0, SEEK_SET);
                if (kread(fd, filebuf, filesize)!=filesize)
                    { kclose(fd); Bfree(highpaldata); initprintf("Error: didn't read all of \"%s\".\n", fn); break; }

                kclose(fd);
                kpgetdim(filebuf, filesize, &xsiz, &ysiz);

                if (EDUKE32_PREDICT_FALSE(xsiz != PR_HIGHPALOOKUP_DIM*PR_HIGHPALOOKUP_DIM || ysiz != PR_HIGHPALOOKUP_DIM))
                {
                    initprintf("Error: image dimensions of \"%s\" must be %dx%d.\n",
                               fn, PR_HIGHPALOOKUP_DIM*PR_HIGHPALOOKUP_DIM, PR_HIGHPALOOKUP_DIM);
                    Bfree(filebuf); Bfree(highpaldata);
                    break;
                }

                i = kprender(filebuf, filesize, (intptr_t)highpaldata, xsiz*sizeof(coltype), xsiz, ysiz);
                Bfree(filebuf);
                if (EDUKE32_PREDICT_FALSE(i))
                    { Bfree(highpaldata); initprintf("Error: failed rendering \"%s\".\n", fn); break; }
            }

            polymer_definehighpalookup(basepal, pal, highpaldata);

            Bfree(highpaldata);
#endif
        }
        break;
        case T_TINT:
        {
            char *tinttokptr = script->ltextptr;
            int32_t red=255, green=255, blue=255, shadered=0, shadegreen=0, shadeblue=0, pal=-1, flags=0;
            char *tintend;

            static const tokenlist tinttokens[] =
            {
                { "pal",        T_PAL        },
                { "red",        T_RED        },{ "r",  T_RED },
                { "green",      T_GREEN      },{ "g",  T_GREEN },
                { "blue",       T_BLUE       },{ "b",  T_BLUE },
                { "shadered",   T_SHADERED   },{ "sr", T_SHADERED },
                { "shadegreen", T_SHADEGREEN },{ "sg", T_SHADEGREEN },
                { "shadeblue",  T_SHADEBLUE  },{ "sb", T_SHADEBLUE },
                { "flags",      T_FLAGS      }
            };

            if (scriptfile_getbraces(script,&tintend)) break;
            while (script->textptr < tintend)
            {
                switch (getatoken(script,tinttokens,ARRAY_SIZE(tinttokens)))
                {
                case T_PAL:
                    scriptfile_getsymbol(script,&pal);        break;
                case T_RED:
                    scriptfile_getnumber(script,&red);        red        = min(255,max(0,red));   break;
                case T_GREEN:
                    scriptfile_getnumber(script,&green);      green      = min(255,max(0,green)); break;
                case T_BLUE:
                    scriptfile_getnumber(script,&blue);       blue       = min(255,max(0,blue));  break;
                case T_SHADERED:
                    scriptfile_getnumber(script,&shadered);   shadered   = min(255,max(0,shadered));   break;
                case T_SHADEGREEN:
                    scriptfile_getnumber(script,&shadegreen); shadegreen = min(255,max(0,shadegreen)); break;
                case T_SHADEBLUE:
                    scriptfile_getnumber(script,&shadeblue);  shadeblue  = min(255,max(0,shadeblue));  break;
                case T_FLAGS:
                    scriptfile_getsymbol(script,&flags);      break;
                }
            }

            if (EDUKE32_PREDICT_FALSE(pal < 0))
            {
                initprintf("Error: tint: missing 'palette number' near line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,tinttokptr));
                break;
            }

#ifdef USE_OPENGL
            hicsetpalettetint(pal,red,green,blue,shadered,shadegreen,shadeblue,flags);
#endif
        }
        break;
        case T_MAKEPALOOKUP:
        {
            char *const starttokptr = script->ltextptr;
            int32_t red=0, green=0, blue=0, pal=-1;
            int32_t havepal=0, remappal=0;
            int32_t nofloorpal=-1;
            char *endtextptr;

            static const tokenlist palookuptokens[] =
            {
                { "pal",   T_PAL },
                { "red",   T_RED   }, { "r", T_RED },
                { "green", T_GREEN }, { "g", T_GREEN },
                { "blue",  T_BLUE  }, { "b", T_BLUE },
                { "remappal", T_REMAPPAL },
                { "remapself", T_REMAPSELF },
                { "nofloorpal", T_NOFLOORPAL },
            };

            enum {
                HAVE_PAL = 1,
                HAVE_REMAPPAL = 2,
                HAVE_REMAPSELF = 4,

                HAVEPAL_SPECIAL = HAVE_REMAPPAL | HAVE_REMAPSELF,
                HAVEPAL_ERROR = 8,
            };

            if (scriptfile_getbraces(script,&endtextptr)) break;
            while (script->textptr < endtextptr)
            {
                switch (getatoken(script, palookuptokens, ARRAY_SIZE(palookuptokens)))
                {
                case T_PAL:
                    scriptfile_getsymbol(script, &pal);
                    havepal |= HAVE_PAL;
                    break;
                case T_RED:
                    scriptfile_getnumber(script,&red);
                    red = clamp(red, 0, 63);
                    break;
                case T_GREEN:
                    scriptfile_getnumber(script,&green);
                    green = clamp(green, 0, 63);
                    break;
                case T_BLUE:
                    scriptfile_getnumber(script,&blue);
                    blue = clamp(blue, 0, 63);
                    break;
                case T_REMAPPAL:
                    scriptfile_getsymbol(script,&remappal);
                    if (havepal & HAVEPAL_SPECIAL)
                        havepal |= HAVEPAL_ERROR;
                    havepal |= HAVE_REMAPPAL;
                    break;
                case T_REMAPSELF:
                    if (havepal & HAVEPAL_SPECIAL)
                        havepal |= HAVEPAL_ERROR;
                    havepal |= HAVE_REMAPSELF;
                    break;
                case T_NOFLOORPAL:
                    scriptfile_getsymbol(script, &nofloorpal);
                    nofloorpal = clamp(nofloorpal, 0, 1);
                    break;
                }
            }

            {
                char msgend[BMAX_PATH+64];

                Bsprintf(msgend, "for palookup definition near line %s:%d",
                         script->filename, scriptfile_getlinum(script,starttokptr));

                if (EDUKE32_PREDICT_FALSE((havepal & HAVE_PAL)==0))
                {
                    initprintf("Error: missing 'palette number' %s\n", msgend);
                    break;
                }
                else if (EDUKE32_PREDICT_FALSE(pal==0 || (unsigned)pal >= MAXPALOOKUPS-RESERVEDPALS))
                {
                    initprintf("Error: 'palette number' out of range (1 .. %d) %s\n",
                               MAXPALOOKUPS-RESERVEDPALS-1, msgend);
                    break;
                }

                if (EDUKE32_PREDICT_FALSE(havepal & HAVEPAL_ERROR))
                {
                    // will also disallow multiple remappals or remapselfs
                    initprintf("Error: must have exactly one of either 'remappal' or 'remapself' %s\n", msgend);
                    break;
                }
                else if (EDUKE32_PREDICT_FALSE((havepal & HAVE_REMAPPAL)
                                               && (unsigned)remappal >= MAXPALOOKUPS-RESERVEDPALS))
                {
                    initprintf("Error: 'remap palette number' out of range (max=%d) %s\n",
                               MAXPALOOKUPS-RESERVEDPALS-1, msgend);
                    break;
                }

                if (havepal & HAVE_REMAPSELF)
                    remappal = pal;
            }

            // NOTE: all palookups are initialized, i.e. non-NULL!
            // NOTE2: aliasing (pal==remappal) is OK
            makepalookup(pal, palookup[remappal], red<<2, green<<2, blue<<2,
                         remappal==0 ? 1 : (nofloorpal == -1 ? g_noFloorPal[remappal] : nofloorpal));
        }
        break;
        case T_TEXTURE:
        {
            char *texturetokptr = script->ltextptr, *textureend;
            int32_t tile=-1, token;

            static const tokenlist texturetokens[] =
            {
                { "pal",     T_PAL  },
                { "detail",  T_DETAIL },
                { "glow",    T_GLOW },
                { "specular",T_SPECULAR },
                { "normal",  T_NORMAL },
            };

            if (scriptfile_getsymbol(script,&tile)) break;
            if (scriptfile_getbraces(script,&textureend)) break;
            while (script->textptr < textureend)
            {
                token = getatoken(script,texturetokens,ARRAY_SIZE(texturetokens));
                switch (token)
                {
                case T_PAL:
                {
                    char *paltokptr = script->ltextptr, *palend;
                    int32_t pal=-1, xsiz = 0, ysiz = 0;
                    char *fn = NULL;
                    double alphacut = -1.0, xscale = 1.0, yscale = 1.0, specpower = 1.0, specfactor = 1.0;
#ifdef USE_OPENGL
                    char flags = 0;
#endif

                    static const tokenlist texturetokens_pal[] =
                    {
                        { "file",            T_FILE },{ "name", T_FILE },
                        { "alphacut",        T_ALPHACUT },
                        { "detailscale",     T_XSCALE }, { "scale",  T_XSCALE }, { "xscale",  T_XSCALE }, { "intensity",  T_XSCALE },
                        { "yscale",          T_YSCALE },
                        { "specpower",       T_SPECPOWER }, { "specularpower", T_SPECPOWER }, { "parallaxscale", T_SPECPOWER },
                        { "specfactor",      T_SPECFACTOR }, { "specularfactor", T_SPECFACTOR }, { "parallaxbias", T_SPECFACTOR },
                        { "nocompress",      T_NOCOMPRESS },
                        { "nodownsize",      T_NODOWNSIZE },
                        { "forcefilter",     T_FORCEFILTER },
                        { "artquality",      T_ARTQUALITY },
                        { "orig_sizex",      T_ORIGSIZEX }, { "orig_sizey", T_ORIGSIZEY }
                    };

                    if (scriptfile_getsymbol(script,&pal)) break;
                    if (scriptfile_getbraces(script,&palend)) break;
                    while (script->textptr < palend)
                    {
                        switch (getatoken(script,texturetokens_pal,ARRAY_SIZE(texturetokens_pal)))
                        {
                        case T_FILE:
                            scriptfile_getstring(script,&fn); break;
                        case T_ALPHACUT:
                            scriptfile_getdouble(script,&alphacut); break;
                        case T_XSCALE:
                            scriptfile_getdouble(script,&xscale); break;
                        case T_YSCALE:
                            scriptfile_getdouble(script,&yscale); break;
                        case T_SPECPOWER:
                            scriptfile_getdouble(script,&specpower); break;
                        case T_SPECFACTOR:
                            scriptfile_getdouble(script,&specfactor); break;
#ifdef USE_OPENGL
                        case T_NOCOMPRESS:
                            flags |= HICR_NOTEXCOMPRESS; break;
                        case T_NODOWNSIZE:
                            flags |= HICR_NODOWNSIZE; break;
                        case T_FORCEFILTER:
                            flags |= HICR_FORCEFILTER; break;
                        case T_ARTQUALITY:
                            flags |= HICR_ARTIMMUNITY; break;
#endif
                        case T_ORIGSIZEX:
                            scriptfile_getnumber(script, &xsiz);
                            break;
                        case T_ORIGSIZEY:
                            scriptfile_getnumber(script, &ysiz);
                            break;
                        default:
                            break;
                        }
                    }

                    if (EDUKE32_PREDICT_FALSE((unsigned)tile >= MAXUSERTILES)) break;	// message is printed later
                    if (EDUKE32_PREDICT_FALSE((unsigned)pal >= MAXPALOOKUPS - RESERVEDPALS))
                    {
                        initprintf("Error: missing or invalid 'palette number' for texture definition near "
                                   "line %s:%d\n", script->filename, scriptfile_getlinum(script,paltokptr));
                        break;
                    }
                    if (EDUKE32_PREDICT_FALSE(!fn))
                    {
                        initprintf("Error: missing 'file name' for texture definition near line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,paltokptr));
                        break;
                    }

                    if (EDUKE32_PREDICT_FALSE(check_file_exist(fn)))
                        break;

                    if (xsiz > 0 && ysiz > 0)
                    {
                        set_tilesiz(tile, xsiz, ysiz);
                        Bmemset(&picanm[tile], 0, sizeof(picanm_t));
                        E_CreateDummyTile(tile);
                    }
#ifdef USE_OPENGL
                    xscale = 1.0f / xscale;
                    yscale = 1.0f / yscale;

                    hicsetsubsttex(tile,pal,fn,alphacut,xscale,yscale, specpower, specfactor,flags);
#endif
                }
                break;
                case T_DETAIL: case T_GLOW: case T_SPECULAR: case T_NORMAL:
                {
                    char *detailtokptr = script->ltextptr, *detailend;
#ifdef USE_OPENGL
                    int32_t pal = 0;
                    char flags = 0;
#endif
                    char *fn = NULL;
                    double xscale = 1.0, yscale = 1.0, specpower = 1.0, specfactor = 1.0;

                    static const tokenlist texturetokens_pal[] =
                    {
                        { "file",            T_FILE },{ "name", T_FILE },
                        { "alphacut",        T_ALPHACUT },
                        { "detailscale",     T_XSCALE }, { "scale",  T_XSCALE }, { "xscale",  T_XSCALE }, { "intensity",  T_XSCALE },
                        { "yscale",          T_YSCALE },
                        { "specpower",       T_SPECPOWER }, { "specularpower", T_SPECPOWER }, { "parallaxscale", T_SPECPOWER },
                        { "specfactor",      T_SPECFACTOR }, { "specularfactor", T_SPECFACTOR }, { "parallaxbias", T_SPECFACTOR },
                        { "nocompress",      T_NOCOMPRESS },
                        { "nodownsize",      T_NODOWNSIZE },
                        { "forcefilter",     T_FORCEFILTER },
                        { "artquality",      T_ARTQUALITY },
                    };

                    if (EDUKE32_PREDICT_FALSE(scriptfile_getbraces(script,&detailend))) break;
                    while (script->textptr < detailend)
                    {
                        switch (getatoken(script,texturetokens_pal,ARRAY_SIZE(texturetokens_pal)))
                        {
                        case T_FILE:
                            scriptfile_getstring(script,&fn); break;
                        case T_XSCALE:
                            scriptfile_getdouble(script,&xscale); break;
                        case T_YSCALE:
                            scriptfile_getdouble(script,&yscale); break;
                        case T_SPECPOWER:
                            scriptfile_getdouble(script,&specpower); break;
                        case T_SPECFACTOR:
                            scriptfile_getdouble(script,&specfactor); break;
#ifdef USE_OPENGL
                        case T_NOCOMPRESS:
                            flags |= HICR_NOTEXCOMPRESS; break;
                        case T_NODOWNSIZE:
                            flags |= HICR_NODOWNSIZE; break;
                        case T_FORCEFILTER:
                            flags |= HICR_FORCEFILTER; break;
                        case T_ARTQUALITY:
                            flags |= HICR_ARTIMMUNITY; break;
#endif
                        default:
                            break;
                        }
                    }

                    if (EDUKE32_PREDICT_FALSE((unsigned)tile >= MAXUSERTILES)) break;	// message is printed later
                    if (EDUKE32_PREDICT_FALSE(!fn))
                    {
                        initprintf("Error: missing 'file name' for texture definition near line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,detailtokptr));
                        break;
                    }

                    if (EDUKE32_PREDICT_FALSE(check_file_exist(fn)))
                        break;

#ifdef USE_OPENGL
                    switch (token)
                    {
                    case T_DETAIL:
                        pal = DETAILPAL;
                        xscale = 1.0f / xscale;
                        yscale = 1.0f / yscale;
                        break;
                    case T_GLOW:
                        pal = GLOWPAL;
                        break;
                    case T_SPECULAR:
                        pal = SPECULARPAL;
                        break;
                    case T_NORMAL:
                        pal = NORMALPAL;
                        break;
                    }
                    hicsetsubsttex(tile,pal,fn,-1.0f,xscale,yscale, specpower, specfactor,flags);
#endif
                }
                break;
                default:
                    break;
                }
            }
            if (EDUKE32_PREDICT_FALSE((unsigned)tile >= MAXUSERTILES))
            {
                initprintf("Error: missing or invalid 'tile number' for texture definition near line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,texturetokptr));
                break;
            }
        }
        break;

        case T_UNDEFMODEL:
        case T_UNDEFMODELRANGE:
        {
            int32_t r0,r1;

            if (EDUKE32_PREDICT_FALSE(scriptfile_getsymbol(script,&r0))) break;
            if (tokn == T_UNDEFMODELRANGE)
            {
                if (scriptfile_getsymbol(script,&r1)) break;

                if (check_tile_range("undefmodelrange", &r0, &r1, script, cmdtokptr))
                    break;
            }
            else
            {
                r1 = r0;

                if (check_tile("undefmodel", r0, script, cmdtokptr))
                    break;
            }
#ifdef USE_OPENGL
            for (; r0 <= r1; r0++)
                md_undefinetile(r0);
#endif
        }
        break;

        case T_UNDEFMODELOF:
        {
            int32_t r0;
#if defined USE_OPENGL && 0
            int32_t mid;
#endif

            if (EDUKE32_PREDICT_FALSE(scriptfile_getsymbol(script,&r0))) break;

            if (check_tile("undefmodelof", r0, script, cmdtokptr))
                break;

            // XXX: See comment of md_undefinemodel()
            initprintf("Warning: undefmodelof: currently non-functional.\n");
            break;

#if defined USE_OPENGL && 0
            mid = md_tilehasmodel(r0,0);
            if (mid < 0) break;

            md_undefinemodel(mid);
#endif
        }
        break;

        case T_UNDEFTEXTURE:
        case T_UNDEFTEXTURERANGE:
        {
            int32_t r0,r1;
#ifdef USE_OPENGL
            int32_t i;
#endif

            if (EDUKE32_PREDICT_FALSE(scriptfile_getsymbol(script,&r0))) break;
            if (tokn == T_UNDEFTEXTURERANGE)
            {
                if (EDUKE32_PREDICT_FALSE(scriptfile_getsymbol(script,&r1))) break;

                if (EDUKE32_PREDICT_FALSE(check_tile_range("undeftexturerange", &r0, &r1, script, cmdtokptr)))
                    break;
            }
            else
            {
                r1 = r0;

                if (EDUKE32_PREDICT_FALSE(check_tile("undeftexture", r0, script, cmdtokptr)))
                    break;
            }

#ifdef USE_OPENGL
            for (; r0 <= r1; r0++)
                for (i=MAXPALOOKUPS-1; i>=0; i--)
                    hicclearsubst(r0,i);
#endif
        }
        break;

        case T_CUTSCENE:
        case T_ANIMSOUNDS:
        {
            char *dummy;

            static const tokenlist dummytokens[] = { { "id",   T_ID  }, };

            if (EDUKE32_PREDICT_FALSE(scriptfile_getstring(script, &dummy))) break;
            if (EDUKE32_PREDICT_FALSE(scriptfile_getbraces(script,&dummy))) break;
            while (script->textptr < dummy)
            {
                // XXX?
                getatoken(script,dummytokens,sizeof(dummytokens)/sizeof(dummytokens));
            }
        }
        break;

        case T_TEXHITSCANRANGE:
        case T_NOFULLBRIGHTRANGE:
        {
            int32_t b,e, i;

            if (EDUKE32_PREDICT_FALSE(scriptfile_getnumber(script,&b))) break;
            if (EDUKE32_PREDICT_FALSE(scriptfile_getnumber(script,&e))) break;

            b = max(b, 0);
            e = min(e, MAXUSERTILES-1);

            for (i=b; i<=e; i++)
                picanm[i].sf |= (tokn==T_TEXHITSCANRANGE) ?
                    PICANM_TEXHITSCAN_BIT : PICANM_NOFULLBRIGHT_BIT;
        }
        break;

        case T_SOUND:
        case T_MUSIC:
        {
            char *dummy, *dummy2;
            static const tokenlist sound_musictokens[] =
            {
                { "id",   T_ID  },
                { "file", T_FILE },
            };

            if (EDUKE32_PREDICT_FALSE(scriptfile_getbraces(script,&dummy))) break;
            while (script->textptr < dummy)
            {
                switch (getatoken(script,sound_musictokens,ARRAY_SIZE(sound_musictokens)))
                {
                case T_ID:
                    scriptfile_getstring(script,&dummy2);
                    break;
                case T_FILE:
                    scriptfile_getstring(script,&dummy2);
                    break;
                }
            }
        }
        break;

        case T_MAPINFO:
        {
            char *mapmd4string = NULL, *title = NULL, *mhkfile = NULL, *mapinfoend, *dummy;
            static const tokenlist mapinfotokens[] =
            {
                { "mapfile",    T_MAPFILE },
                { "maptitle",   T_MAPTITLE },
                { "mapmd4",     T_MAPMD4 },
                { "mhkfile",    T_MHKFILE },
            };
            int32_t previous_usermaphacks = num_usermaphacks;

            if (EDUKE32_PREDICT_FALSE(scriptfile_getbraces(script,&mapinfoend))) break;
            while (script->textptr < mapinfoend)
            {
                switch (getatoken(script,mapinfotokens,ARRAY_SIZE(mapinfotokens)))
                {
                case T_MAPFILE:
                    scriptfile_getstring(script,&dummy);
                    break;
                case T_MAPTITLE:
                    scriptfile_getstring(script,&title);
                    break;
                case T_MAPMD4:
                {
                    scriptfile_getstring(script,&mapmd4string);

                    num_usermaphacks++;
                    usermaphacks = (usermaphack_t *)Xrealloc(usermaphacks, num_usermaphacks*sizeof(usermaphack_t));
                    usermaphack_t *newusermaphack = &usermaphacks[num_usermaphacks - 1];

                    for (bssize_t i = 0; i < 16; i++)
                    {
                        char smallbuf[3] = { 0, 0, 0 };
                        smallbuf[0] = mapmd4string[2*i];
                        smallbuf[1] = mapmd4string[2*i+1];
                        newusermaphack->md4[i] = Bstrtol(smallbuf, NULL, 16);
                    }

                    break;
                }
                case T_MHKFILE:
                    scriptfile_getstring(script,&mhkfile);
                    break;
                }
            }

            for (; previous_usermaphacks < num_usermaphacks; previous_usermaphacks++)
            {
                usermaphacks[previous_usermaphacks].mhkfile = mhkfile ? Bstrdup(mhkfile) : NULL;
                usermaphacks[previous_usermaphacks].title = title ? Bstrdup(title) : NULL;
            }
        }
        break;

        case T_ECHO:
        {
            char *string = NULL;
            scriptfile_getstring(script,&string);
            initprintf("%s\n",string);
        }
        break;

        case T_GLOBALFLAGS:
        {
            if (scriptfile_getnumber(script,&globalflags)) break;
        }
        break;

        case T_GLOBALGAMEFLAGS:
        {
            int32_t dummy;
            if (scriptfile_getnumber(script,&dummy)) break;
        }
        break;

        case T_MULTIPSKY:
        {
            char *blockend;
            int32_t tile;

            static const tokenlist subtokens[] =
            {
                { "horizfrac",       T_HORIZFRAC },
                { "yoffset",         T_YOFFSET },
                { "lognumtiles",     T_LOGNUMTILES },
                { "tile",            T_TILE },
                { "panel",           T_TILE },
                { "yscale",          T_YSCALE },
            };

            if (scriptfile_getsymbol(script,&tile))
                break;
            if (scriptfile_getbraces(script,&blockend))
                break;

            if (tile != DEFAULTPSKY && (unsigned)tile >= MAXUSERTILES)
            {
                script->textptr = blockend;
                break;
            }

            psky_t * const newpsky = E_DefinePsky(tile);

            while (script->textptr < blockend)
            {
                int32_t token = getatoken(script,subtokens,ARRAY_SIZE(subtokens));
                switch (token)
                {
                case T_HORIZFRAC:
                {
                    int32_t horizfrac;
                    scriptfile_getsymbol(script,&horizfrac);

                    newpsky->horizfrac = horizfrac;
                    break;
                }
                case T_YOFFSET:
                {
                    int32_t yoffset;
                    scriptfile_getsymbol(script,&yoffset);

                    newpsky->yoffs = yoffset;
                    break;
                }
                case T_LOGNUMTILES:
                {
                    int32_t lognumtiles;
                    scriptfile_getsymbol(script,&lognumtiles);

                    if ((1<<lognumtiles) > MAXPSKYTILES)
                        break;

                    newpsky->lognumtiles = lognumtiles;
                    break;
                }
                case T_TILE:
                {
                    int32_t panel, offset;
                    scriptfile_getsymbol(script,&panel);
                    scriptfile_getsymbol(script,&offset);

                    if ((unsigned) panel >= MAXPSKYTILES)
                        break;

                    if ((unsigned) offset > PSKYOFF_MAX)
                        break;

                    newpsky->tileofs[panel] = offset;
                    break;
                }
                case T_YSCALE:
                {
                    int32_t yscale;
                    scriptfile_getsymbol(script,&yscale);

                    newpsky->yscale = yscale;
                    break;
                }
                default:
                    break;
                }
            }
        }
        break;
        case T_BASEPALETTE:
        {
            char *blockend;
            int32_t id;

            static const tokenlist subtokens[] =
            {
                { "raw",         T_RAW },
                { "copy",        T_COPY },
                { "undef",       T_UNDEF },
            };

            if (scriptfile_getsymbol(script,&id))
                break;
            if (scriptfile_getbraces(script,&blockend))
                break;

            if (EDUKE32_PREDICT_FALSE((unsigned)id >= MAXBASEPALS))
            {
                initprintf("Error: basepalette: Invalid basepal number on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                script->textptr = blockend;
                break;
            }

            int didLoadPal = 0;

            while (script->textptr < blockend)
            {
                int32_t token = getatoken(script,subtokens,ARRAY_SIZE(subtokens));
                switch (token)
                {
                case T_RAW:
                {
                    char *rawblockend;

                    static const tokenlist rawsubtokens[] =
                    {
                        { "file",           T_FILE },
                        { "offset",         T_OFFSET },
                        { "shiftleft",      T_SHIFTLEFT },
                    };

                    if (scriptfile_getbraces(script,&rawblockend))
                        break;

                    char * fn = NULL;
                    int32_t offset = 0;
                    int32_t shiftleft = 0;

                    while (script->textptr < rawblockend)
                    {
                        int32_t token = getatoken(script,rawsubtokens,ARRAY_SIZE(rawsubtokens));
                        switch (token)
                        {
                        case T_FILE:
                        {
                            scriptfile_getstring(script,&fn);
                            break;
                        }
                        case T_OFFSET:
                        {
                            scriptfile_getnumber(script,&offset);
                            break;
                        }
                        case T_SHIFTLEFT:
                        {
                            scriptfile_getnumber(script,&shiftleft);
                            break;
                        }
                        default:
                            break;
                        }
                    }

                    if (EDUKE32_PREDICT_FALSE(fn == NULL))
                    {
                        initprintf("Error: basepalette: No filename provided on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    if (EDUKE32_PREDICT_FALSE(offset < 0))
                    {
                        initprintf("Error: basepalette: Invalid file offset on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    if (EDUKE32_PREDICT_FALSE((unsigned)shiftleft >= 8))
                    {
                        initprintf("Error: basepalette: Invalid left shift provided on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    int32_t const fil = kopen4load(fn, 0);
                    if (EDUKE32_PREDICT_FALSE(fil == -1))
                    {
                        initprintf("Error: basepalette: Failed opening \"%s\" on line %s:%d\n", fn,
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    if (klseek_and_test(fil, offset, BSEEK_SET))
                    {
                        initprintf("Error: basepalette: Seek failed on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        kclose(fil);
                        break;
                    }

                    uint8_t * const palbuf = (uint8_t *)Xmalloc(768);
                    if (kread_and_test(fil,palbuf,768))
                    {
                        initprintf("Error: basepalette: Read failed on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        Bfree(palbuf);
                        kclose(fil);
                        break;
                    }

                    if (shiftleft != 0)
                    {
                        for (bssize_t k = 0; k < 768; k++)
                            palbuf[k] <<= shiftleft;
                    }

                    setbasepal(id, palbuf);
                    didLoadPal = 1;

                    Bfree(palbuf);
                    kclose(fil);
                    break;
                }
                case T_COPY:
                {
                    int32_t source;
                    scriptfile_getsymbol(script,&source);

                    if (EDUKE32_PREDICT_FALSE((unsigned)source >= MAXBASEPALS || source == id))
                    {
                        initprintf("Error: basepalette: Invalid source basepal number on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    uint8_t const * const sourcetable = basepaltable[source];
                    if (EDUKE32_PREDICT_FALSE(sourcetable == NULL))
                    {
                        initprintf("Error: basepalette: Source basepal does not exist on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    setbasepal(id, sourcetable);
                    didLoadPal = 1;
                    break;
                }
                case T_UNDEF:
                {
                    removebasepal(id);

                    didLoadPal = 0;
                    if (id == 0)
                        paletteloaded &= ~PALETTE_MAIN;
                    break;
                }
                default:
                    break;
                }
            }

            if (didLoadPal && id == 0)
            {
                initfastcolorlookup_palette(palette);

                paletteloaded |= PALETTE_MAIN;
            }
        }
        break;
        case T_PALOOKUP:
        {
            char *blockend;
            int32_t id;

            static const tokenlist subtokens[] =
            {
                { "raw",            T_RAW },
                { "copy",           T_COPY },
                { "undef",          T_UNDEF },

                { "fogpal",         T_FOGPAL },
                { "makepalookup",   T_MAKEPALOOKUP },

                { "floorpal",       T_FLOORPAL },
                { "nofloorpal",     T_NOFLOORPAL },
            };

            if (scriptfile_getsymbol(script,&id))
                break;
            if (scriptfile_getbraces(script,&blockend))
                break;

            if (EDUKE32_PREDICT_FALSE((unsigned)id >= MAXPALOOKUPS))
            {
                initprintf("Error: palookup: Invalid pal number on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                script->textptr = blockend;
                break;
            }

            int didLoadShade = 0;

            while (script->textptr < blockend)
            {
                int32_t token = getatoken(script,subtokens,ARRAY_SIZE(subtokens));
                switch (token)
                {
                case T_RAW:
                {
                    char *subblockend;

                    static const tokenlist rawsubtokens[] =
                    {
                        { "file",           T_FILE },
                        { "offset",         T_OFFSET },
                        { "noshades",       T_NOSHADES },
                    };

                    if (scriptfile_getbraces(script,&subblockend))
                        break;

                    char * fn = NULL;
                    int32_t offset = 0;
                    int32_t length = 256*32; // hardcoding 32 instead of numshades

                    while (script->textptr < subblockend)
                    {
                        int32_t token = getatoken(script,rawsubtokens,ARRAY_SIZE(rawsubtokens));
                        switch (token)
                        {
                        case T_FILE:
                        {
                            scriptfile_getstring(script,&fn);
                            break;
                        }
                        case T_OFFSET:
                        {
                            scriptfile_getnumber(script,&offset);
                            break;
                        }
                        case T_NOSHADES:
                        {
                            length = 256;
                            break;
                        }
                        default:
                            break;
                        }
                    }

                    if (EDUKE32_PREDICT_FALSE(fn == NULL))
                    {
                        initprintf("Error: palookup: No filename provided on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    if (EDUKE32_PREDICT_FALSE(offset < 0))
                    {
                        initprintf("Error: palookup: Invalid file offset on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    int32_t const fil = kopen4load(fn, 0);
                    if (EDUKE32_PREDICT_FALSE(fil == -1))
                    {
                        initprintf("Error: palookup: Failed opening \"%s\" on line %s:%d\n", fn,
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    if (klseek_and_test(fil, offset, BSEEK_SET))
                    {
                        initprintf("Error: palookup: Seek failed on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        kclose(fil);
                        break;
                    }

                    char * const palookupbuf = (char *)Xmalloc(length);
                    int32_t bytesread = kread(fil, palookupbuf, length);
                    if (bytesread < 256)
                    {
                        initprintf("Error: palookup: Read failed on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        Bfree(palookupbuf);
                        kclose(fil);
                        break;
                    }

                    if (bytesread == 256*32)
                    {
                        didLoadShade = 1;
                        numshades = 32;
                        setpalookup(id, (uint8_t *)palookupbuf);
                    }
                    else
                    {
                        if (EDUKE32_PREDICT_FALSE(!(paletteloaded & PALETTE_SHADE)))
                        {
                            initprintf("Error: palookup: Shade tables not loaded on line %s:%d\n",
                                       script->filename, scriptfile_getlinum(script,cmdtokptr));
                            break;
                        }

                        makepalookup(id, palookupbuf, 0,0,0, g_noFloorPal[id]);
                    }

                    Bfree(palookupbuf);
                    kclose(fil);
                    break;
                }
                case T_COPY:
                {
                    int32_t source;
                    scriptfile_getsymbol(script,&source);

                    if (EDUKE32_PREDICT_FALSE((unsigned)source >= MAXPALOOKUPS || source == id))
                    {
                        initprintf("Error: palookup: Invalid source pal number on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    if (EDUKE32_PREDICT_FALSE(source == 0 && !(paletteloaded & PALETTE_SHADE)))
                    {
                        initprintf("Error: palookup: Shade tables not loaded on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    uint8_t const * const sourcepal = (uint8_t *)palookup[source];
                    if (EDUKE32_PREDICT_FALSE(sourcepal == NULL))
                    {
                        initprintf("Error: palookup: Source palookup does not exist on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    setpalookup(id, sourcepal);
                    didLoadShade = 1;
                    break;
                }
                case T_FOGPAL:
                {
                    char *subblockend;

                    static const tokenlist fogpaltokens[] =
                    {
                        { "red",   T_RED   }, { "r", T_RED },
                        { "green", T_GREEN }, { "g", T_GREEN },
                        { "blue",  T_BLUE  }, { "b", T_BLUE },
                    };

                    int32_t red = 0, green = 0, blue = 0;

                    if (scriptfile_getbraces(script,&subblockend))
                        break;

                    while (script->textptr < subblockend)
                    {
                        switch (getatoken(script, fogpaltokens, ARRAY_SIZE(fogpaltokens)))
                        {
                        case T_RED:
                            scriptfile_getnumber(script,&red);
                            red = clamp(red, 0, 255);
                            break;
                        case T_GREEN:
                            scriptfile_getnumber(script,&green);
                            green = clamp(green, 0, 255);
                            break;
                        case T_BLUE:
                            scriptfile_getnumber(script,&blue);
                            blue = clamp(blue, 0, 255);
                            break;
                        }
                    }

                    if (EDUKE32_PREDICT_FALSE(!(paletteloaded & PALETTE_SHADE)))
                    {
                        initprintf("Error: palookup: Shade tables not loaded on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    makepalookup(id, NULL, red, green, blue, 1);
                    break;
                }
                case T_MAKEPALOOKUP:
                {
                    char *subblockend;

                    static const tokenlist makepalookuptokens[] =
                    {
                        { "red",   T_RED   }, { "r", T_RED },
                        { "green", T_GREEN }, { "g", T_GREEN },
                        { "blue",  T_BLUE  }, { "b", T_BLUE },
                        { "remappal", T_REMAPPAL },
                        { "remapself", T_REMAPSELF },
                    };

                    int32_t red = 0, green = 0, blue = 0;
                    int32_t remappal = -1;

                    if (scriptfile_getbraces(script,&subblockend))
                        break;

                    while (script->textptr < subblockend)
                    {
                        switch (getatoken(script, makepalookuptokens, ARRAY_SIZE(makepalookuptokens)))
                        {
                        case T_RED:
                            scriptfile_getnumber(script,&red);
                            red = clamp(red, 0, 255);
                            break;
                        case T_GREEN:
                            scriptfile_getnumber(script,&green);
                            green = clamp(green, 0, 255);
                            break;
                        case T_BLUE:
                            scriptfile_getnumber(script,&blue);
                            blue = clamp(blue, 0, 255);
                            break;
                        case T_REMAPPAL:
                            scriptfile_getsymbol(script,&remappal);
                            break;
                        case T_REMAPSELF:
                            remappal = id;
                            break;
                        }
                    }

                    if (EDUKE32_PREDICT_FALSE((unsigned)remappal >= MAXPALOOKUPS))
                    {
                        initprintf("Error: palookup: Invalid remappal on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    if (EDUKE32_PREDICT_FALSE(!(paletteloaded & PALETTE_SHADE)))
                    {
                        initprintf("Error: palookup: Shade tables not loaded on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    makepalookup(id, NULL, red, green, blue, g_noFloorPal[id]);

                    break;
                }
                case T_NOFLOORPAL:
                {
                    g_noFloorPal[id] = 1;
                    break;
                }
                case T_FLOORPAL:
                {
                    g_noFloorPal[id] = 0;
                    break;
                }
                case T_UNDEF:
                {
                    removepalookup(id);

                    didLoadShade = 0;
                    if (id == 0)
                        paletteloaded &= ~PALETTE_SHADE;
                    break;
                }
                default:
                    break;
                }
            }

            if (didLoadShade && id == 0)
            {
                paletteloaded |= PALETTE_SHADE;
            }
        }
        break;
        case T_BLENDTABLE:
        {
            char *blockend;
            int32_t id;

            static const tokenlist subtokens[] =
            {
                { "raw",         T_RAW },
                { "glblend",     T_GLBLEND },
                { "copy",        T_COPY },
                { "undef",       T_UNDEF },
            };

            if (scriptfile_getsymbol(script,&id))
                break;
            if (scriptfile_getbraces(script,&blockend))
                break;

            if (EDUKE32_PREDICT_FALSE((unsigned)id >= MAXBLENDTABS))
            {
                initprintf("Error: blendtable: Invalid blendtable number on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                script->textptr = blockend;
                break;
            }

            int didLoadTransluc = 0;

            while (script->textptr < blockend)
            {
                int32_t token = getatoken(script,subtokens,ARRAY_SIZE(subtokens));
                switch (token)
                {
                case T_RAW:
                {
                    char *rawblockend;

                    static const tokenlist rawsubtokens[] =
                    {
                        { "file",           T_FILE },
                        { "offset",         T_OFFSET },
                    };

                    if (scriptfile_getbraces(script,&rawblockend))
                        break;

                    char * fn = NULL;
                    int32_t offset = 0;

                    while (script->textptr < rawblockend)
                    {
                        int32_t token = getatoken(script,rawsubtokens,ARRAY_SIZE(rawsubtokens));
                        switch (token)
                        {
                        case T_FILE:
                        {
                            scriptfile_getstring(script,&fn);
                            break;
                        }
                        case T_OFFSET:
                        {
                            scriptfile_getnumber(script,&offset);
                            break;
                        }
                        default:
                            break;
                        }
                    }

                    if (EDUKE32_PREDICT_FALSE(fn == NULL))
                    {
                        initprintf("Error: blendtable: No filename provided on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    if (EDUKE32_PREDICT_FALSE(offset < 0))
                    {
                        initprintf("Error: blendtable: Invalid file offset on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    int32_t const fil = kopen4load(fn, 0);
                    if (EDUKE32_PREDICT_FALSE(fil == -1))
                    {
                        initprintf("Error: blendtable: Failed opening \"%s\" on line %s:%d\n", fn,
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    if (klseek_and_test(fil, offset, BSEEK_SET))
                    {
                        initprintf("Error: blendtable: Seek failed on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        kclose(fil);
                        break;
                    }

                    char * const blendbuf = (char *)Xmalloc(256*256);
                    if (kread_and_test(fil,blendbuf,256*256))
                    {
                        initprintf("Error: blendtable: Read failed on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        Bfree(blendbuf);
                        kclose(fil);
                        break;
                    }

                    setblendtab(id, blendbuf);
                    didLoadTransluc = 1;

                    Bfree(blendbuf);
                    kclose(fil);
                    break;
                }
                case T_COPY:
                {
                    int32_t source;
                    scriptfile_getsymbol(script,&source);

                    if (EDUKE32_PREDICT_FALSE((unsigned)source >= MAXBLENDTABS || source == id))
                    {
                        initprintf("Error: blendtable: Invalid source blendtable number on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    char const * const sourcetable = blendtable[source];
                    if (EDUKE32_PREDICT_FALSE(sourcetable == NULL))
                    {
                        initprintf("Error: blendtable: Source blendtable does not exist on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,cmdtokptr));
                        break;
                    }

                    setblendtab(id, sourcetable);
                    didLoadTransluc = 1;

#ifdef USE_OPENGL
                    glblend[id] = glblend[source];
#endif
                    break;
                }
                case T_UNDEF:
                {
                    removeblendtab(id);

                    didLoadTransluc = 0;
                    if (id == 0)
                        paletteloaded &= ~PALETTE_TRANSLUC;

#ifdef USE_OPENGL
                    glblend[id] = defaultglblend;
#endif
                    break;
                }
                case T_GLBLEND:
                {
                    char *glblendblockend;

                    static const tokenlist glblendtokens[] =
                    {
                        { "forward",     T_FORWARD },
                        { "reverse",     T_REVERSE },
                        { "both",        T_BOTH },
                    };

                    if (scriptfile_getbraces(script,&glblendblockend))
                        break;

#ifdef USE_OPENGL
                    glblend_t * const glb = glblend + id;
                    *glb = nullglblend;
#endif

                    while (script->textptr < glblendblockend)
                    {
                        int32_t glblendtoken = getatoken(script,glblendtokens,ARRAY_SIZE(glblendtokens));
                        switch (glblendtoken)
                        {
                        case T_FORWARD:
                        case T_REVERSE:
                        case T_BOTH:
                        {
                            char *glblenddefblockend;

                            static const tokenlist glblenddeftokens[] =
                            {
                                { "src",         T_SRC },
                                { "sfactor",     T_SRC },
                                { "top",         T_SRC },

                                { "dst",         T_DST },
                                { "dfactor",     T_DST },
                                { "bottom",      T_DST },

                                { "alpha",       T_ALPHA },
                            };

                            if (scriptfile_getbraces(script,&glblenddefblockend))
                                break;

#ifdef USE_OPENGL
                            glblenddef_t * const glbdef = glb->def + (glblendtoken == T_REVERSE);
#endif

                            while (script->textptr < glblenddefblockend)
                            {
                                int32_t glblenddeftoken = getatoken(script,glblenddeftokens,ARRAY_SIZE(glblenddeftokens));
                                switch (glblenddeftoken)
                                {
                                case T_SRC:
                                case T_DST:
                                {
                                    static const tokenlist glBlendFuncTokens[] =
                                    {
                                        { "ZERO", T_ZERO },
                                        { "ONE", T_ONE },
                                        { "SRC_COLOR", T_SRC_COLOR },
                                        { "ONE_MINUS_SRC_COLOR", T_ONE_MINUS_SRC_COLOR },
                                        { "SRC_ALPHA", T_SRC_ALPHA },
                                        { "ONE_MINUS_SRC_ALPHA", T_ONE_MINUS_SRC_ALPHA },
                                        { "DST_ALPHA", T_DST_ALPHA },
                                        { "ONE_MINUS_DST_ALPHA", T_ONE_MINUS_DST_ALPHA },
                                        { "DST_COLOR", T_DST_COLOR },
                                        { "ONE_MINUS_DST_COLOR", T_ONE_MINUS_DST_COLOR },
                                    };

                                    int32_t factortoken = getatoken(script,glBlendFuncTokens,ARRAY_SIZE(glBlendFuncTokens));

#ifdef USE_OPENGL
                                    uint8_t * const factor = glblenddeftoken == T_SRC ? &glbdef->src : &glbdef->dst;
                                    switch (factortoken)
                                    {
                                        case T_ZERO: *factor = BLENDFACTOR_ZERO; break;
                                        case T_ONE: *factor = BLENDFACTOR_ONE; break;
                                        case T_SRC_COLOR: *factor = BLENDFACTOR_SRC_COLOR; break;
                                        case T_ONE_MINUS_SRC_COLOR: *factor = BLENDFACTOR_ONE_MINUS_SRC_COLOR; break;
                                        case T_SRC_ALPHA: *factor = BLENDFACTOR_SRC_ALPHA; break;
                                        case T_ONE_MINUS_SRC_ALPHA: *factor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA; break;
                                        case T_DST_ALPHA: *factor = BLENDFACTOR_DST_ALPHA; break;
                                        case T_ONE_MINUS_DST_ALPHA: *factor = BLENDFACTOR_ONE_MINUS_DST_ALPHA; break;
                                        case T_DST_COLOR: *factor = BLENDFACTOR_DST_COLOR; break;
                                        case T_ONE_MINUS_DST_COLOR: *factor = BLENDFACTOR_ONE_MINUS_DST_COLOR; break;
                                    }
#else
                                    UNREFERENCED_PARAMETER(factortoken);
#endif

                                    break;
                                }
                                case T_ALPHA:
                                {
                                    double tempalpha;
                                    scriptfile_getdouble(script,&tempalpha);
#ifdef USE_OPENGL
                                    glbdef->alpha = (float)tempalpha;
#endif
                                    break;
                                }
                                }
                            }

#ifdef USE_OPENGL
                            if (glblendtoken == T_BOTH)
                                glb->def[1] = *glbdef;
#endif

                            break;
                        }
                        }
                    }
                }
                default:
                    break;
                }
            }

            if (didLoadTransluc && id == 0)
            {
                paletteloaded |= PALETTE_TRANSLUC;
            }
        }
        break;
        case T_NUMALPHATABS:
        {
            int32_t value;
            if (scriptfile_getnumber(script,&value)) break;

            switch (value)
            {
                case 1: case 3: case 7: case 15: case 31: case 63: case 127:
                case 2: case 4: case 8: case 16: case 32: case 64: case 128:
#ifdef USE_OPENGL
                    for (int32_t a = 1, value2 = value*2 + (value&1); a <= value; ++a)
                    {
                        float finv2value = 1.f/(float)value2;

                        glblend_t * const glb = glblend + a;
                        *glb = defaultglblend;
                        glb->def[0].alpha = (float)(value2-a) * finv2value;
                        glb->def[1].alpha = (float)a * finv2value;
                    }
                    fallthrough__;
#endif
                case 0:
                    numalphatabs = value;
                    break;
                default:
                    initprintf("Error: numalphatables: Invalid value on line %s:%d\n",
                               script->filename, scriptfile_getlinum(script,cmdtokptr));
                    break;
            }
        }
        break;
        case T_UNDEFBASEPALETTERANGE:
        {
            int32_t id0, id1;

            if (scriptfile_getsymbol(script,&id0))
                break;
            if (scriptfile_getsymbol(script,&id1))
                break;

            if (EDUKE32_PREDICT_FALSE(id0 > id1 || (unsigned)id0 >= MAXBASEPALS || (unsigned)id1 >= MAXBASEPALS))
            {
                initprintf("Error: undefbasepaletterange: Invalid range on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }

            for (bssize_t i = id0; i <= id1; i++)
                removebasepal(i);

            if (id0 == 0)
                paletteloaded &= ~PALETTE_MAIN;
        }
        break;
        case T_UNDEFPALOOKUPRANGE:
        {
            int32_t id0, id1;

            if (scriptfile_getsymbol(script,&id0))
                break;
            if (scriptfile_getsymbol(script,&id1))
                break;

            if (EDUKE32_PREDICT_FALSE(id0 > id1 || (unsigned)id0 >= MAXPALOOKUPS || (unsigned)id1 >= MAXPALOOKUPS))
            {
                initprintf("Error: undefpalookuprange: Invalid range on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }

            for (bssize_t i = id0; i <= id1; i++)
                removepalookup(i);

            if (id0 == 0)
                paletteloaded &= ~PALETTE_SHADE;
        }
        break;
        case T_UNDEFBLENDTABLERANGE:
        {
            int32_t id0, id1;

            if (scriptfile_getsymbol(script,&id0))
                break;
            if (scriptfile_getsymbol(script,&id1))
                break;

            if (EDUKE32_PREDICT_FALSE(id0 > id1 || (unsigned)id0 >= MAXBLENDTABS || (unsigned)id1 >= MAXBLENDTABS))
            {
                initprintf("Error: undefblendtablerange: Invalid range on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }

            for (bssize_t i = id0; i <= id1; i++)
                removeblendtab(i);

            if (id0 == 0)
                paletteloaded &= ~PALETTE_TRANSLUC;
        }
        break;

        default:
            initprintf("Unknown token.\n"); break;
        }
    }

    return 0;
}


int32_t loaddefinitionsfile(const char *fn)
{
    scriptfile *script;
    int32_t f = flushlogwindow;
    int32_t i;

    script = scriptfile_fromfile(fn);

    if (script)
    {
        flushlogwindow = 1;
        initprintf("Loading \"%s\"\n",fn);
        flushlogwindow = 0;

        defsparser(script);
    }

    for (i=0; i < g_defModulesNum; ++i)
        defsparser_include(g_defModules[i], NULL, NULL);

    flushlogwindow = f;

    if (script)
        scriptfile_close(script);

    scriptfile_clearsymbols();

    DO_FREE_AND_NULL(faketilebuffer);
    faketilebuffersiz = 0;

    if (usermaphacks != NULL)
        qsort(usermaphacks, num_usermaphacks, sizeof(usermaphack_t), compare_usermaphacks);

    if (!script) return -1;

    initprintf("\n");

    return 0;
}

// vim:ts=4:
