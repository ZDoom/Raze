/*
 * Definitions file parser for Build
 * by Jonathon Fowler (jf@jonof.id.au)
 * Remixed substantially by Ken Silverman
 * See the included license file "BUILDLIC.TXT" for license info.
 */

#include "build.h"
#include "compat.h"
#include "engine_priv.h"
#include "scriptfile.h"

#include "mdsprite.h"  // md3model_t
#include "buildtiles.h"
#include "bitmap.h"
#include "m_argv.h"
#include "gamecontrol.h"
#include "palettecontainer.h"
#include "mapinfo.h"
#include "hw_voxels.h"
#include "parsefuncs.h"

int32_t getatoken(scriptfile *sf, const tokenlist *tl, int32_t ntokens)
{
    int32_t i;

    if (!sf) return T_ERROR;
	if (!sf->GetString()) return T_EOF;

    for (i=ntokens-1; i>=0; i--)
    {
        if (sf->Compare(tl[i].text))
            return tl[i].tokenid;
    }
    return T_ERROR;
}

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
    T_SHADEFACTOR,
    T_IFCRC,T_IFMATCH,T_CRC32,
    T_SIZE,
    T_NEWGAMECHOICES,
    T_RFFDEFINEID,
    T_EXTRA,
    T_ROTATE,
    T_SURFACE, T_VIEW,
};

static int32_t defsparser(scriptfile *script);

static void defsparser_include(const char *fn, scriptfile *script, FScriptPosition *pos)
{
    scriptfile *included;

    included = scriptfile_fromfile(fn);
    if (!included)
    {
        if (!pos)
            Printf("Warning: Failed including %s as module\n", fn);
        else
			pos->Message(MSG_ERROR, "Failed including %s", fn);
    }
    else
    {
		if (script) included->symbols = std::move(script->symbols);
        defsparser(included);
		if (script) script->symbols = std::move(included->symbols);
        scriptfile_close(included);
    }
}


static int32_t check_tile_range(const char *defcmd, int32_t *tilebeg, int32_t *tileend,
                                const scriptfile *script, FScriptPosition pos)
{
    if (*tileend < *tilebeg)
    {
        pos.Message(MSG_WARNING, "%s: backwards tile range", defcmd);
        std::swap(*tilebeg, *tileend);
    }

    if ((unsigned)*tilebeg >= MAXUSERTILES || (unsigned)*tileend >= MAXUSERTILES)
    {
        pos.Message(MSG_ERROR, "%s: Invalid tile range", defcmd);
        return 1;
    }

    return 0;
}

static int32_t check_tile(const char *defcmd, int32_t tile, const scriptfile *script, FScriptPosition pos)
{
    if ((unsigned)tile >= MAXUSERTILES)
    {
        pos.Message(MSG_ERROR, "%s: Invalid tile number", defcmd);
        return 1;
    }

    return 0;
}

#undef USE_DEF_PROGRESS
#if defined _WIN32 || defined HAVE_GTK2
# define USE_DEF_PROGRESS
#endif

static int32_t defsparser(scriptfile *script)
{
    int32_t tokn;
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
        { "shadefactor",     T_SHADEFACTOR      },
        { "newgamechoices",  T_NEWGAMECHOICES   },
        { "rffdefineid",     T_RFFDEFINEID      },  // dummy
    };

    script->SetNoOctals(true);
    while (1)
    {
#ifdef USE_DEF_PROGRESS
        if (++iter >= 50)
        {
            Printf(".");
            iter = 0;
        }
#endif
        tokn = getatoken(script,basetokens,countof(basetokens));
		auto pos = scriptfile_getposition(script);
        switch (tokn)
        {
        case T_ERROR:
            pos.Message(MSG_ERROR, "Unknown error");
            break;
        case T_EOF:
            return 0;
        case T_INCLUDE:
        {
            FString fn;
            if (!scriptfile_getstring(script,&fn))
                defsparser_include(fn, script, &pos);
            break;
        }
        case T_INCLUDEDEFAULT:
            defsparser_include(G_DefaultDefFile(), script, &pos);
            break;
        case T_LOADGRP:
        case T_CACHESIZE:
        case T_SHADEFACTOR:
        case T_GLOBALGAMEFLAGS:
        case T_GLOBALFLAGS:
            parseSkip<1>(*script, pos);
            break;
        case T_UNDEFBLENDTABLERANGE:
            parseSkip<2>(*script, pos);
            break;
        case T_SPRITECOL:
        case T_2DCOLIDXRANGE:  // NOTE: takes precedence over 2dcol, see InitCustomColors()
            parseSkip<3>(*script, pos);
            break;
        case T_2DCOL:
            parseSkip<4>(*script, pos);
            break;
        case T_CUTSCENE:
        case T_ANIMSOUNDS:
            parseEmptyBlockWithParm(*script, pos);;
            break;
        case T_NEWGAMECHOICES: // stub
            parseEmptyBlock(*script, pos);
            break;

        case T_DEFINE:
            parseDefine(*script, pos);
            break;
        case T_DEFINETEXTURE:
            parseDefineTexture(*script, pos);
            break;
        case T_DEFINESKYBOX:
            parseDefineSkybox(*script, pos);
            break;
        case T_DEFINETINT:
            parseDefineTint(*script, pos);
            break;
        case T_ALPHAHACK:
            parseAlphahack(*script, pos);
            break;
        case T_ALPHAHACKRANGE:
            parseAlphahackRange(*script, pos);
            break;
        case T_FOGPAL:
            parseFogpal(*script, pos);
            break;
        case T_NOFLOORPALRANGE:
            parseNoFloorpalRange(*script, pos);
            break;
        case T_ARTFILE:
            parseArtFile(*script, pos);
            break;
        case T_SETUPTILE:
            parseSetupTile(*script, pos);
            break;

        case T_SETUPTILERANGE:
            parseSetupTileRange(*script, pos);
            break;

        case T_ANIMTILERANGE:
            parseAnimTileRange(*script, pos);
            break;

        case T_TILEFROMTEXTURE:
            parseTileFromTexture(*script, pos);
            break;
        case T_COPYTILE:
            parseCopyTile(*script, pos);
            break;
        case T_IMPORTTILE:
            parseImportTile(*script, pos);
            break;
        case T_DUMMYTILE:
            parseDummyTile(*script, pos);
            break;
        case T_DUMMYTILERANGE:
            parseDummyTileRange(*script, pos);
            break;
        case T_UNDEFINETILE:
            parseUndefineTile(*script, pos);
            break;
        case T_UNDEFINETILERANGE:
            parseUndefineTileRange(*script, pos);
            break;

        case T_DEFINEMODEL:
            parseDefineModel(*script, pos);
            break;
        case T_DEFINEMODELFRAME:
            parseDefineModelFrame(*script, pos);
            break;
        case T_DEFINEMODELANIM:
            parseDefineModelAnim(*script, pos);
            break;
        case T_DEFINEMODELSKIN:
            parseDefineModelSkin(*script, pos);
            break;
        case T_SELECTMODELSKIN:
            parseSelectModelSkin(*script, pos);
            break;
        case T_DEFINEVOXEL:
            parseDefineVoxel(*script, pos);
            break;
        case T_DEFINEVOXELTILES:
            parseDefineVoxelTiles(*script, pos);
            break;

        // NEW (ENCOURAGED) DEFINITION SYNTAX
        case T_MODEL:
            parseModel(*script, pos);
        break;
        case T_VOXEL:
            parseVoxel(*script, pos);
            break;
        case T_SKYBOX:
            parseSkybox(*script, pos);
            break;
        case T_HIGHPALOOKUP:
            parseHighpalookup(*script, pos);
            break;
        case T_TINT:
            parseTint(*script, pos);
            break;
        case T_MAKEPALOOKUP:
            parseMakePalookup(*script, pos);
            break;
        case T_TEXTURE:
            parseTexture(*script, pos);
            break;

        case T_UNDEFMODEL:
            parseUndefModel(*script, pos);
            break;
        case T_UNDEFMODELRANGE:
            parseUndefModelRange(*script, pos);
            break;

        case T_UNDEFMODELOF:
            parseUndefModelOf(*script, pos);
            break;

        case T_UNDEFTEXTURE:
            parseUndefTexture(*script, pos);
            break;
        case T_UNDEFTEXTURERANGE:
            parseUndefTextureRange(*script, pos);
            break;

        case T_TEXHITSCANRANGE:
            parseTexHitscanRange(*script, pos);
            break;
        case T_NOFULLBRIGHTRANGE:
            parseNoFullbrightRange(*script, pos);
        break;

        case T_SOUND:
        case T_MUSIC:
            parseMusic(*script, pos);
        break;

        case T_MAPINFO:
            parseMapinfo(*script, pos);
            break;

        case T_ECHO:
            parseEcho(*script, pos);
            break;

        case T_MULTIPSKY:
            parseMultiPsky(*script, pos);
            break;
        case T_BASEPALETTE:
            parseBasePalette(*script, pos);
        break;
        case T_PALOOKUP:
            parsePalookup(*script, pos);
            break;
        case T_BLENDTABLE:
            parseBlendTable(*script, pos);
            break;
        case T_NUMALPHATABS:
            parseNumAlphaTabs(*script, pos);
            break;
        case T_UNDEFBASEPALETTERANGE:
            parseUndefBasePaletteRange(*script, pos);
            break;
        case T_UNDEFPALOOKUPRANGE:
            parseUndefPalookupRange(*script, pos);
            break;
        case T_RFFDEFINEID:
            parseRffDefineId(*script, pos);
        break;


        default:
            pos.Message(MSG_ERROR, "%s: Unknown token.", script->String); break;
        }
    }

    return 0;
}

int32_t loaddefinitionsfile(const char *fn, bool loadadds, bool cumulative)
{
    bool done = false;
    auto parseit = [&](int lump)
    {
        FScanner sc;
        sc.OpenLumpNum(lump);
        sc.SetNoOctals(true);
        sc.SetNoFatalErrors(true);
        defsparser(&sc);
        done = true;
        Printf(PRINT_NONOTIFY, "\n");
    };

    if (!cumulative)
    {
        int lump = fileSystem.FindFile(fn);
        if (lump >= 0)
        {
            Printf(PRINT_NONOTIFY, "Loading \"%s\"\n", fn);
            parseit(lump);
    }
    }
    else
    {
        int lump, lastlump = 0;
        while ((lump = fileSystem.FindLumpFullName(fn, &lastlump)) >= 0)
        {
            Printf(PRINT_NONOTIFY, "Loading \"%s\"\n", fileSystem.GetFileFullPath(lump).GetChars());
            parseit(lump);
        }
    }

    if (userConfig.AddDefs && loadadds) for (auto& m : *userConfig.AddDefs)
	{
		Printf("Loading module \"%s\"\n",m.GetChars());
        defsparser_include(m, nullptr, nullptr); // Q: should we let the external script see our symbol table?
        Printf(PRINT_NONOTIFY, "\n");
	}
    return 0;
}
