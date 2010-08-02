/*
 * Definitions file parser for Build
 * by Jonathon Fowler (jonof@edgenetwork.org)
 * Remixed substantially by Ken Silverman
 * See the included license file "BUILDLIC.TXT" for license info.
 */

#include "build.h"
#include "compat.h"
#include "baselayer.h"
#include "scriptfile.h"
#include "cache1d.h"
#include "kplib.h"
#include "quicklz.h"

enum scripttoken_t
{
    T_EOF = -2,
    T_ERROR = -1,
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
    T_FLIPPED,
    T_HIDE,
    T_NOBOB,
    T_NODEPTH,
    T_VOXEL,
    T_SKYBOX,
    T_FRONT,T_RIGHT,T_BACK,T_LEFT,T_TOP,T_BOTTOM,
    T_TINT,T_RED,T_GREEN,T_BLUE,
    T_TEXTURE,T_ALPHACUT,T_XSCALE,T_YSCALE,T_SPECPOWER,T_SPECFACTOR,T_NOCOMPRESS,T_NODOWNSIZE,
    T_UNDEFMODEL,T_UNDEFMODELRANGE,T_UNDEFMODELOF,T_UNDEFTEXTURE,T_UNDEFTEXTURERANGE,
    T_ALPHAHACK,T_ALPHAHACKRANGE,
    T_SPRITECOL,T_2DCOL,
    T_FOGPAL,
    T_LOADGRP,
    T_DUMMYTILE,T_DUMMYTILERANGE,
    T_SETUPTILE,T_SETUPTILERANGE,
    T_ANIMTILERANGE,
    T_CACHESIZE,
    T_IMPORTTILE,
    T_MUSIC,T_ID,T_SOUND,
    T_TILEFROMTEXTURE, T_XOFFSET, T_YOFFSET
};

typedef struct { char *text; int32_t tokenid; } tokenlist;

static int32_t getatoken(scriptfile *sf, const tokenlist *tl, int32_t ntokens)
{
    char *tok;
    int32_t i;

    if (!sf) return T_ERROR;
    tok = scriptfile_gettoken(sf);
    if (!tok) return T_EOF;

    for (i=ntokens-1; i>=0; i--)
    {
        if (!Bstrcasecmp(tok, tl[i].text))
            return tl[i].tokenid;
    }

    return T_ERROR;
}

static int32_t lastmodelid = -1, lastvoxid = -1, modelskin = -1, lastmodelskin = -1, seenframe = 0;
extern int32_t nextvoxid;

#if defined(POLYMOST) && defined(USE_OPENGL)
extern float alphahackarray[MAXTILES];
#endif

static const char *skyfaces[6] =
{
    "front face", "right face", "back face",
    "left face", "top face", "bottom face"
};

static int32_t defsparser(scriptfile *script)
{
    int32_t tokn;
    char *cmdtokptr;

    static const tokenlist basetokens[] =
    {
        { "include",         T_INCLUDE          },
        { "#include",        T_INCLUDE          },
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
        { "tint",            T_TINT             },
        { "texture",         T_TEXTURE          },
        { "tile",            T_TEXTURE          },
        { "music",           T_MUSIC            },
        { "sound",           T_SOUND            },

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
        { "fogpal",	     	 T_FOGPAL	 		},
        { "loadgrp",     	 T_LOADGRP	 		},
        { "dummytile",     	 T_DUMMYTILE		},
        { "dummytilerange",  T_DUMMYTILERANGE   },
        { "setuptile",       T_SETUPTILE        },
        { "setuptilerange",  T_SETUPTILERANGE   },
        { "animtilerange",   T_ANIMTILERANGE    },
        { "cachesize",       T_CACHESIZE        },
        { "dummytilefrompic",T_IMPORTTILE       },
        { "tilefromtexture", T_TILEFROMTEXTURE  },
    };

    while (1)
    {
        if (quitevent) return 0;
        tokn = getatoken(script,basetokens,sizeof(basetokens)/sizeof(tokenlist));
        cmdtokptr = script->ltextptr;
        switch (tokn)
        {
        case T_ERROR:
            initprintf("Error on line %s:%d.\n", script->filename,scriptfile_getlinum(script,cmdtokptr));
            break;
        case T_EOF:
            return(0);
        case T_INCLUDE:
        {
            char *fn;
            if (!scriptfile_getstring(script,&fn))
            {
                scriptfile *included;

                included = scriptfile_fromfile(fn);
                if (!included)
                {
                    initprintf("Warning: Failed including %s on line %s:%d\n",
                               fn, script->filename,scriptfile_getlinum(script,cmdtokptr));
                }
                else
                {
                    defsparser(included);
                    scriptfile_close(included);
                }
            }
            break;
        }
        case T_DEFINE:
        {
            char *name;
            int32_t number;

            if (scriptfile_getstring(script,&name)) break;
            if (scriptfile_getsymbol(script,&number)) break;

            if (scriptfile_addsymbolvalue(name,number) < 0)
                initprintf("Warning: Symbol %s was NOT redefined to %d on line %s:%d\n",
                           name,number,script->filename,scriptfile_getlinum(script,cmdtokptr));
            break;
        }

        // OLD (DEPRECATED) DEFINITION SYNTAX
        case T_DEFINETEXTURE:
        {
            int32_t tile,pal,fnoo,i;
            char *fn, *tfn = NULL;

            if (scriptfile_getsymbol(script,&tile)) break;
            if (scriptfile_getsymbol(script,&pal))  break;
            if (scriptfile_getnumber(script,&fnoo)) break; //x-center
            if (scriptfile_getnumber(script,&fnoo)) break; //y-center
            if (scriptfile_getnumber(script,&fnoo)) break; //x-size
            if (scriptfile_getnumber(script,&fnoo)) break; //y-size
            if (scriptfile_getstring(script,&fn))  break;

            i = pathsearchmode;
            pathsearchmode = 1;
            if (findfrompath(fn,&tfn) < 0)
            {
                char buf[BMAX_PATH];

                Bstrcpy(buf,fn);
                kzfindfilestart(buf);
                if (!kzfindfile(buf))
                {
                    initprintf("Error: file '%s' does not exist\n",fn);
                    pathsearchmode = i;
                    break;
                }
            }
            else Bfree(tfn);
            pathsearchmode = i;

            hicsetsubsttex(tile,pal,fn,-1.0,1.0,1.0,1.0,1.0,0);
        }
        break;
        case T_DEFINESKYBOX:
        {
            int32_t tile,pal,i,ii;
            char *fn[6],happy=1,*tfn = NULL;

            if (scriptfile_getsymbol(script,&tile)) break;
            if (scriptfile_getsymbol(script,&pal)) break;
            if (scriptfile_getsymbol(script,&i)) break; //future expansion
            for (i=0; i<6; i++)
            {
                if (scriptfile_getstring(script,&fn[i])) break; //grab the 6 faces
                ii = pathsearchmode;
                pathsearchmode = 1;
                if (findfrompath(fn[i],&tfn) < 0)
                {
                    char buf[BMAX_PATH];

                    Bstrcpy(buf,fn[i]);
                    kzfindfilestart(buf);
                    if (!kzfindfile(buf))
                    {
                        initprintf("Error: file '%s' does not exist\n",fn[i]);
                        happy = 0;
                    }
                }
                else Bfree(tfn);
                pathsearchmode = ii;
            }
            if (i < 6 || !happy) break;
            hicsetskybox(tile,pal,fn);
        }
        break;
        case T_DEFINETINT:
        {
            int32_t pal, r,g,b,f;

            if (scriptfile_getsymbol(script,&pal)) break;
            if (scriptfile_getnumber(script,&r)) break;
            if (scriptfile_getnumber(script,&g)) break;
            if (scriptfile_getnumber(script,&b)) break;
            if (scriptfile_getnumber(script,&f)) break; //effects
            hicsetpalettetint(pal,r,g,b,f);
        }
        break;
        case T_ALPHAHACK:
        {
            int32_t tile;
            double alpha;

            if (scriptfile_getsymbol(script,&tile)) break;
            if (scriptfile_getdouble(script,&alpha)) break;
#if defined(POLYMOST) && defined(USE_OPENGL)
            if ((uint32_t)tile < MAXTILES) alphahackarray[tile] = alpha;
#endif
        }
        break;
        case T_ALPHAHACKRANGE:
        {
            int32_t tilenume1,tilenume2,i;
            double alpha;

            if (scriptfile_getsymbol(script,&tilenume1)) break;
            if (scriptfile_getsymbol(script,&tilenume2)) break;
            if (scriptfile_getdouble(script,&alpha)) break;
            if (tilenume2 < tilenume1)
            {
                initprintf("Warning: backwards tile range on line %s:%d\n", script->filename, scriptfile_getlinum(script,cmdtokptr));
                i = tilenume2;
                tilenume2 = tilenume1;
                tilenume1 = i;
            }
#if defined(POLYMOST) && defined(USE_OPENGL)
            if ((tilenume1 >= 0 && tilenume1 < MAXTILES) && (tilenume2 >= 0 && tilenume2 < MAXTILES))
            {
                for (i=tilenume1; i<=tilenume2; i++)
                {
                    if ((uint32_t)i < MAXTILES)
                        alphahackarray[i] = alpha;
                }
            }
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

            if (col < 256)
            {
                vgapal16[col*4+0] = b; // blue
                vgapal16[col*4+1] = g; // green
                vgapal16[col*4+2] = r; // red
            }
        }
        break;
        case T_FOGPAL:
        {
            int32_t p,r,g,b,j;
            char tempbuf[256];

            if (scriptfile_getnumber(script,&p)) break;
            if (scriptfile_getnumber(script,&r)) break;
            if (scriptfile_getnumber(script,&g)) break;
            if (scriptfile_getnumber(script,&b)) break;

            for (j = 0; j < 256; j++)
                tempbuf[j] = j;
            makepalookup(p, tempbuf, r, g, b, 1);
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
        case T_SETUPTILE:
        {
            int32_t tile, tmp;

            if (scriptfile_getsymbol(script,&tile)) break;
            if (tile >= MAXTILES)break;
            if (scriptfile_getsymbol(script,&h_xsize[tile])) break;
            if (scriptfile_getsymbol(script,&h_ysize[tile])) break;
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
            if (tile2 < tile1)
            {
                initprintf("Warning: backwards tile range on line %s:%d\n", script->filename, scriptfile_getlinum(script,cmdtokptr));
                i = tile2;
                tile2 = tile1;
                tile1 = i;
            }
            if ((tile1 >= 0 && tile1 < MAXTILES) && (tile2 >= 0 && tile2 < MAXTILES))
            {
                for (i=tile1; i<=tile2; i++)
                {
                    if ((uint32_t)i < MAXTILES)
                    {
                        h_xsize[i] = xsiz;
                        h_ysize[i] = ysiz;
                        h_xoffs[i] = xoffs;
                        h_yoffs[i] = yoffs;
                    }
                }
            }
            break;
        }
        case T_ANIMTILERANGE:
        {
            int32_t tile1, tile2, spd, type, i;

            if (scriptfile_getsymbol(script,&tile1)) break;
            if (tile1 >= MAXTILES)break;
            if (scriptfile_getsymbol(script,&tile2)) break;
            if (tile2 >= MAXTILES)break;
            if (scriptfile_getsymbol(script,&spd)) break;
            if (scriptfile_getsymbol(script,&type)) break;
            if (tile2 < tile1)
            {
                initprintf("Warning: backwards tile range on line %s:%d\n", script->filename, scriptfile_getlinum(script,cmdtokptr));
                i = tile2;
                tile2 = tile1;
                tile1 = i;
            }
            picanm[tile1]=(picanm[tile1]&0xffffff3f)+(spd<<24)+(type<<6)+tile2-tile1;
            break;
        }
        case T_TILEFROMTEXTURE:
        {
            char *texturetokptr = script->ltextptr, *textureend, *fn, *tfn = NULL, *ftd = NULL;
            int32_t tile=-1, token, i;
            int32_t alphacut = 255;
            int32_t xoffset = 0, yoffset = 0;

            static const tokenlist tilefromtexturetokens[] =
            {
                { "file",            T_FILE },
                { "name",            T_FILE },
                { "alphacut",        T_ALPHACUT },
                { "xoffset",         T_XOFFSET },
                { "xoff",            T_XOFFSET },
                { "yoffset",         T_YOFFSET },
                { "yoff",            T_YOFFSET },
            };

            if (scriptfile_getsymbol(script,&tile)) break;
            if (scriptfile_getbraces(script,&textureend)) break;
            while (script->textptr < textureend)
            {
                token = getatoken(script,tilefromtexturetokens,sizeof(tilefromtexturetokens)/sizeof(tokenlist));
                switch (token)
                {
                case T_FILE:
                    scriptfile_getstring(script,&fn); break;
                case T_ALPHACUT:
                    scriptfile_getsymbol(script,&alphacut); break;
                case T_XOFFSET:
                    scriptfile_getsymbol(script,&xoffset); break;
                case T_YOFFSET:
                    scriptfile_getsymbol(script,&yoffset); break;
                default:
                    break;
                }

                if ((unsigned)tile > (unsigned)MAXTILES) break;	// message is printed later
                if (!fn)
                {
                    initprintf("Error: missing 'file name' for tilefromtexture definition near line %s:%d\n",
                               script->filename, scriptfile_getlinum(script,texturetokptr));
                    break;
                }
                if (alphacut > 255) alphacut = 255;
                if (alphacut < 0) alphacut = 0;

                i = pathsearchmode;
                pathsearchmode = 1;
                if (findfrompath(fn,&tfn) < 0)
                {
                    char buf[BMAX_PATH];

                    Bstrcpy(buf,fn);
                    kzfindfilestart(buf);
                    if (!kzfindfile(buf))
                    {
                        initprintf("Error: file '%s' does not exist\n",fn);
                        pathsearchmode = i;
                        break;
                    }
                }
                else Bfree(tfn);
                pathsearchmode = i;
            }

            if ((unsigned)tile >= (unsigned)MAXTILES)
            {
                initprintf("Error: missing or invalid 'tile number' for texture definition near line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,texturetokptr));
                break;
            }

            {
                int32_t xsiz, ysiz, j;
                int32_t *picptr = NULL;
                palette_t *col;

                kpzload(fn, (intptr_t *)&picptr, &j, &xsiz, &ysiz);

                //            initprintf("got bpl %d xsiz %d ysiz %d\n",bpl,xsiz,ysiz);

                ftd = Bmalloc(xsiz*ysiz);
                faketiledata[tile] = Bmalloc(xsiz*ysiz);

                for (i=xsiz-1; i>=0; i--)
                {
                    for (j=ysiz-1; j>=0; j--)
                    {
                        col = (palette_t *)&picptr[j*xsiz+i];
                        if (col->f < alphacut) { ftd[i*ysiz+j] = 255; continue; }
                        ftd[i*ysiz+j] = getclosestcol(col->b>>2,col->g>>2,col->r>>2);
                    }
                    //                initprintf(" %d %d %d %d\n",col->r,col->g,col->b,col->f);
                }

                if (xsiz > 0 && ysiz > 0)
                {
                    tilesizx[tile] = xsiz;
                    tilesizy[tile] = ysiz;

                    faketilesiz[tile] = qlz_compress(ftd, faketiledata[tile], xsiz*ysiz, state_compress);

                    xoffset = clamp(xoffset, -128, 127);
                    picanm[tile] = (picanm[tile]&0xffff00ff)+((xoffset&255)<<8);
                    yoffset = clamp(yoffset, -128, 127);
                    picanm[tile] = (picanm[tile]&0xff00ffff)+((yoffset&255)<<16);

                    j = 15; while ((j > 1) && (pow2long[j] > xsiz)) j--;
                    picsiz[tile] = ((uint8_t)j);
                    j = 15; while ((j > 1) && (pow2long[j] > ysiz)) j--;
                    picsiz[tile] += ((uint8_t)(j<<4));
                }

                Bfree(picptr);
                Bfree(ftd);
            }
        }
        break;
        case T_IMPORTTILE:
        {
            int32_t tile, xsiz, ysiz, j, i;
            int32_t *picptr = NULL;
            int32_t bpl;
            char *fn, *ftd = NULL;
            palette_t *col;

            if (scriptfile_getsymbol(script,&tile)) break;
            if (scriptfile_getstring(script,&fn))  break;

            kpzload(fn, (intptr_t *)&picptr, &bpl, &xsiz, &ysiz);

//            initprintf("got bpl %d xsiz %d ysiz %d\n",bpl,xsiz,ysiz);

            ftd = Bmalloc(xsiz*ysiz);
            faketiledata[tile] = Bmalloc(xsiz*ysiz);

            for (i=xsiz-1; i>=0; i--)
            {
                for (j=ysiz-1; j>=0; j--)
                {
                    col = (palette_t *)&picptr[j*xsiz+i];
                    if (col->f != 255) { ftd[i*ysiz+j] = 255; continue; }
                    ftd[i*ysiz+j] = getclosestcol(col->b>>2,col->g>>2,col->r>>2);
                }
//                initprintf(" %d %d %d %d\n",col->r,col->g,col->b,col->f);
            }

            if (xsiz > 0 && ysiz > 0)
            {
                tilesizx[tile] = xsiz;
                tilesizy[tile] = ysiz;
                faketilesiz[tile] = qlz_compress(ftd, faketiledata[tile], xsiz*ysiz, state_compress);
                picanm[tile] = 0;

                j = 15; while ((j > 1) && (pow2long[j] > xsiz)) j--;
                picsiz[tile] = ((uint8_t)j);
                j = 15; while ((j > 1) && (pow2long[j] > ysiz)) j--;
                picsiz[tile] += ((uint8_t)(j<<4));
            }

            Bfree(picptr);
            Bfree(ftd);
            break;
        }
        case T_DUMMYTILE:
        {
            int32_t tile, xsiz, ysiz, j;

            if (scriptfile_getsymbol(script,&tile)) break;
            if (scriptfile_getsymbol(script,&xsiz)) break;
            if (scriptfile_getsymbol(script,&ysiz)) break;

            if (xsiz > 0 && ysiz > 0)
            {
                tilesizx[tile] = xsiz;
                tilesizy[tile] = ysiz;
                faketilesiz[tile] = -1;
                picanm[tile] = 0;

                j = 15; while ((j > 1) && (pow2long[j] > xsiz)) j--;
                picsiz[tile] = ((uint8_t)j);
                j = 15; while ((j > 1) && (pow2long[j] > ysiz)) j--;
                picsiz[tile] += ((uint8_t)(j<<4));
            }

            break;
        }
        case T_DUMMYTILERANGE:
        {
            int32_t tile1,tile2,xsiz,ysiz,i,j;

            if (scriptfile_getnumber(script,&tile1)) break;
            if (scriptfile_getnumber(script,&tile2)) break;
            if (scriptfile_getnumber(script,&xsiz)) break;
            if (scriptfile_getnumber(script,&ysiz)) break;
            if (tile2 < tile1)
            {
                initprintf("Warning: backwards tile range on line %s:%d\n", script->filename, scriptfile_getlinum(script,cmdtokptr));
                i = tile2;
                tile2 = tile1;
                tile1 = i;
            }
            if ((tile1 >= 0 && tile1 < MAXTILES) && (tile2 >= 0 && tile2 < MAXTILES))
            {
                for (i=tile1; i<=tile2; i++)
                {
                    if ((uint32_t)i < MAXTILES)
                    {
                        if (xsiz > 0 && ysiz > 0)
                        {
                            tilesizx[i] = xsiz;
                            tilesizy[i] = ysiz;
                            faketilesiz[i] = -1;
                            picanm[i] = 0;

                            j = 15; while ((j > 1) && (pow2long[j] > xsiz)) j--;
                            picsiz[i] = ((uint8_t)j);
                            j = 15; while ((j > 1) && (pow2long[j] > ysiz)) j--;
                            picsiz[i] += ((uint8_t)(j<<4));
                        }
                    }
                }
            }
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

#if defined(POLYMOST) && defined(USE_OPENGL)
            lastmodelid = md_loadmodel(modelfn);
            if (lastmodelid < 0)
            {
                initprintf("Warning: Failed loading MD2/MD3 model \"%s\"\n", modelfn);
                break;
            }
            md_setmisc(lastmodelid,(float)scale, shadeoffs,0.0,0);
#endif
            modelskin = lastmodelskin = 0;
            seenframe = 0;
        }
        break;
        case T_DEFINEMODELFRAME:
        {
            char *framename;
#if defined(POLYMOST) && defined(USE_OPENGL)
            char happy=1;
#endif
            int32_t ftilenume, ltilenume, tilex;

            if (scriptfile_getstring(script,&framename)) break;
            if (scriptfile_getnumber(script,&ftilenume)) break; //first tile number
            if (scriptfile_getnumber(script,&ltilenume)) break; //last tile number (inclusive)
            if (ltilenume < ftilenume)
            {
                initprintf("Warning: backwards tile range on line %s:%d\n", script->filename, scriptfile_getlinum(script,cmdtokptr));
                tilex = ftilenume;
                ftilenume = ltilenume;
                ltilenume = tilex;
            }

            if (lastmodelid < 0)
            {
                initprintf("Warning: Ignoring frame definition.\n");
                break;
            }
#if defined(POLYMOST) && defined(USE_OPENGL)
            for (tilex = ftilenume; tilex <= ltilenume && happy; tilex++)
            {
                switch (md_defineframe(lastmodelid, framename, tilex, max(0,modelskin), 0.0f,0))
                {
                case 0:
                    break;
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

            if (lastmodelid < 0)
            {
                initprintf("Warning: Ignoring animation definition.\n");
                break;
            }
#if defined(POLYMOST) && defined(USE_OPENGL)
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

#if defined(POLYMOST) && defined(USE_OPENGL)
            switch (md_defineskin(lastmodelid, skinfn, palnum, max(0,modelskin), 0, 0.0f, 1.0f, 1.0f))
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

            if (scriptfile_getstring(script,&fn)) break; //voxel filename

            if (nextvoxid == MAXVOXELS)
            {
                initprintf("Maximum number of voxels already defined.\n");
                break;
            }

#ifdef SUPERBUILD
            if (qloadkvx(nextvoxid, fn))
            {
                initprintf("Failure loading voxel file \"%s\"\n",fn);
                break;
            }

            lastvoxid = nextvoxid++;
#endif
        }
        break;
        case T_DEFINEVOXELTILES:
        {
            int32_t ftilenume, ltilenume, tilex;

            if (scriptfile_getnumber(script,&ftilenume)) break; //1st tile #
            if (scriptfile_getnumber(script,&ltilenume)) break; //last tile #

            if (ltilenume < ftilenume)
            {
                initprintf("Warning: backwards tile range on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                tilex = ftilenume;
                ftilenume = ltilenume;
                ltilenume = tilex;
            }
            if (ltilenume < 0 || ftilenume >= MAXTILES)
            {
                initprintf("Invalid tile range on line %s:%d\n",
                           script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }

            if (lastvoxid < 0)
            {
                initprintf("Warning: Ignoring voxel tiles definition.\n");
                break;
            }
#ifdef SUPERBUILD
            for (tilex = ftilenume; tilex <= ltilenume; tilex++)
            {
                tiletovox[tilex] = lastvoxid;
            }
#endif
        }
        break;

        // NEW (ENCOURAGED) DEFINITION SYNTAX
        case T_MODEL:
        {
            char *modelend, *modelfn;
            double scale=1.0, mzadd=0.0;
            int32_t shadeoffs=0, pal=0, flags=0;

            static const tokenlist modeltokens[] =
            {
                { "scale",    T_SCALE    },
                { "shade",    T_SHADE    },
                { "zadd",     T_ZADD     },
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

            modelskin = lastmodelskin = 0;
            seenframe = 0;

            if (scriptfile_getstring(script,&modelfn)) break;
            if (scriptfile_getbraces(script,&modelend)) break;
#if defined(POLYMOST) && defined(USE_OPENGL)
            lastmodelid = md_loadmodel(modelfn);
            if (lastmodelid < 0)
            {
                initprintf("Warning: Failed loading MD2/MD3 model \"%s\"\n", modelfn);
                script->textptr = modelend+1;
                break;
            }
#endif
            while (script->textptr < modelend)
            {
                int32_t token = getatoken(script,modeltokens,sizeof(modeltokens)/sizeof(tokenlist));
                switch (token)
                {
                    //case T_ERROR: initprintf("Error on line %s:%d in model tokens\n", script->filename,script->linenum); break;
                case T_SCALE:
                    scriptfile_getdouble(script,&scale); break;
                case T_SHADE:
                    scriptfile_getnumber(script,&shadeoffs); break;
                case T_ZADD:
                    scriptfile_getdouble(script,&mzadd); break;
                case T_FLAGS:
                    scriptfile_getnumber(script,&flags); break;
                case T_FRAME:
                {
                    char *frametokptr = script->ltextptr;
                    char *frameend, *framename = 0, happy=1;
                    int32_t ftilenume = -1, ltilenume = -1, tilex = 0;
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
                        switch (getatoken(script,modelframetokens,sizeof(modelframetokens)/sizeof(tokenlist)))
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

                    if (ftilenume < 0) initprintf("Error: missing 'first tile number' for frame definition near line %s:%d\n", script->filename, scriptfile_getlinum(script,frametokptr)), happy = 0;
                    if (ltilenume < 0) initprintf("Error: missing 'last tile number' for frame definition near line %s:%d\n", script->filename, scriptfile_getlinum(script,frametokptr)), happy = 0;
                    if (!happy) break;

                    if (ltilenume < ftilenume)
                    {
                        initprintf("Warning: backwards tile range on line %s:%d\n", script->filename, scriptfile_getlinum(script,frametokptr));
                        tilex = ftilenume;
                        ftilenume = ltilenume;
                        ltilenume = tilex;
                    }

                    if (lastmodelid < 0)
                    {
                        initprintf("Warning: Ignoring frame definition.\n");
                        break;
                    }
#if defined(POLYMOST) && defined(USE_OPENGL)
                    for (tilex = ftilenume; tilex <= ltilenume && happy; tilex++)
                    {
                        switch (md_defineframe(lastmodelid, framename, tilex, max(0,modelskin), smoothduration,pal))
                        {
                        case 0:
                            break;
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
                        }
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
                        switch (getatoken(script,modelanimtokens,sizeof(modelanimtokens)/sizeof(tokenlist)))
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

                    if (!startframe) initprintf("Error: missing 'start frame' for anim definition near line %s:%d\n", script->filename, scriptfile_getlinum(script,animtokptr)), happy = 0;
                    if (!endframe) initprintf("Error: missing 'end frame' for anim definition near line %s:%d\n", script->filename, scriptfile_getlinum(script,animtokptr)), happy = 0;
                    if (!happy) break;

                    if (lastmodelid < 0)
                    {
                        initprintf("Warning: Ignoring animation definition.\n");
                        break;
                    }
#if defined(POLYMOST) && defined(USE_OPENGL)
                    switch (md_defineanimation(lastmodelid, startframe, endframe, (int32_t)(dfps*(65536.0*.001)), flags))
                    {
                    case 0:
                        break;
                    case -1:
                        break; // invalid model id!?
                    case -2:
                        initprintf("Invalid starting frame name on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,animtokptr));
                        break;
                    case -3:
                        initprintf("Invalid ending frame name on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,animtokptr));
                        break;
                    case -4:
                        initprintf("Out of memory on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,animtokptr));
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

                    static const tokenlist modelskintokens[] =
                    {
                        { "pal",           T_PAL        },
                        { "file",          T_FILE       },
                        { "surf",          T_SURF       },
                        { "surface",       T_SURF       },
                        { "intensity",     T_PARAM      },
                        { "scale",         T_PARAM      },
                        { "detailscale",   T_PARAM      },
                        { "specpower",     T_SPECPOWER  }, { "parallaxscale", T_SPECPOWER },
                        { "specfactor",    T_SPECFACTOR }, { "parallaxbias", T_SPECFACTOR },
                    };

                    if (scriptfile_getbraces(script,&skinend)) break;
                    while (script->textptr < skinend)
                    {
                        switch (getatoken(script,modelskintokens,sizeof(modelskintokens)/sizeof(tokenlist)))
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
                        }
                    }

                    if (!skinfn)
                    {
                        initprintf("Error: missing 'skin filename' for skin definition near line %s:%d\n", script->filename, scriptfile_getlinum(script,skintokptr));
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

#if defined(POLYMOST) && defined(USE_OPENGL)
                    switch (md_defineskin(lastmodelid, skinfn, palnum, max(0,modelskin), surfnum, param, specpower, specfactor))
                    {
                    case 0:
                        break;
                    case -1:
                        break; // invalid model id!?
                    case -2:
                        initprintf("Invalid skin filename on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,skintokptr));
                        break;
                    case -3:
                        initprintf("Invalid palette number on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,skintokptr));
                        break;
                    case -4:
                        initprintf("Out of memory on line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,skintokptr));
                        break;
                    }
#endif
                }
                break;
                case T_HUD:
                {
                    char *hudtokptr = script->ltextptr;
                    char happy=1, *frameend;
                    int32_t ftilenume = -1, ltilenume = -1, tilex = 0, flags = 0;
                    double xadd = 0.0, yadd = 0.0, zadd = 0.0, angadd = 0.0;

                    static const tokenlist modelhudtokens[] =
                    {
                        { "tile",   T_TILE   },
                        { "tile0",  T_TILE0  },
                        { "tile1",  T_TILE1  },
                        { "xadd",   T_XADD   },
                        { "yadd",   T_YADD   },
                        { "zadd",   T_ZADD   },
                        { "angadd", T_ANGADD },
                        { "hide",   T_HIDE   },
                        { "nobob",  T_NOBOB  },
                        { "flipped",T_FLIPPED},
                        { "nodepth",T_NODEPTH},
                    };

                    if (scriptfile_getbraces(script,&frameend)) break;
                    while (script->textptr < frameend)
                    {
                        switch (getatoken(script,modelhudtokens,sizeof(modelhudtokens)/sizeof(tokenlist)))
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
                            scriptfile_getdouble(script,&angadd); break;
                        case T_HIDE:
                            flags |= 1; break;
                        case T_NOBOB:
                            flags |= 2; break;
                        case T_FLIPPED:
                            flags |= 4; break;
                        case T_NODEPTH:
                            flags |= 8; break;
                        }
                    }

                    if (ftilenume < 0) initprintf("Error: missing 'first tile number' for hud definition near line %s:%d\n", script->filename, scriptfile_getlinum(script,hudtokptr)), happy = 0;
                    if (ltilenume < 0) initprintf("Error: missing 'last tile number' for hud definition near line %s:%d\n", script->filename, scriptfile_getlinum(script,hudtokptr)), happy = 0;
                    if (!happy) break;

                    if (ltilenume < ftilenume)
                    {
                        initprintf("Warning: backwards tile range on line %s:%d\n", script->filename, scriptfile_getlinum(script,hudtokptr));
                        tilex = ftilenume;
                        ftilenume = ltilenume;
                        ltilenume = tilex;
                    }

                    if (lastmodelid < 0)
                    {
                        initprintf("Warning: Ignoring frame definition.\n");
                        break;
                    }
#if defined(POLYMOST) && defined(USE_OPENGL)
                    for (tilex = ftilenume; tilex <= ltilenume && happy; tilex++)
                    {
                        switch (md_definehud(lastmodelid, tilex, xadd, yadd, zadd, angadd, flags))
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
                    }
#endif
                }
                break;
                }
            }

#if defined(POLYMOST) && defined(USE_OPENGL)
            md_setmisc(lastmodelid,(float)scale,shadeoffs,(float)mzadd,flags);
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

            if (scriptfile_getstring(script,&fn)) break; //voxel filename
            if (nextvoxid == MAXVOXELS) { initprintf("Maximum number of voxels already defined.\n"); break; }
#ifdef SUPERBUILD
            if (qloadkvx(nextvoxid, fn)) { initprintf("Failure loading voxel file \"%s\"\n",fn); break; }
            lastvoxid = nextvoxid++;
#endif

            if (scriptfile_getbraces(script,&modelend)) break;
            while (script->textptr < modelend)
            {
                switch (getatoken(script,voxeltokens,sizeof(voxeltokens)/sizeof(tokenlist)))
                {
                    //case T_ERROR: initprintf("Error on line %s:%d in voxel tokens\n", script->filename,linenum); break;
                case T_TILE:
                    scriptfile_getsymbol(script,&tilex);
#ifdef SUPERBUILD
                    if ((uint32_t)tilex < MAXTILES) tiletovox[tilex] = lastvoxid;
                    else initprintf("Invalid tile number on line %s:%d\n",script->filename, scriptfile_getlinum(script,voxeltokptr));
#endif
                    break;
                case T_TILE0:
                    scriptfile_getsymbol(script,&tile0); break; //1st tile #
                case T_TILE1:
                    scriptfile_getsymbol(script,&tile1);
                    if (tile0 > tile1)
                    {
                        initprintf("Warning: backwards tile range on line %s:%d\n", script->filename, scriptfile_getlinum(script,voxeltokptr));
                        tilex = tile0; tile0 = tile1; tile1 = tilex;
                    }
                    if ((tile1 < 0) || (tile0 >= MAXTILES))
                        { initprintf("Invalid tile range on line %s:%d\n",script->filename, scriptfile_getlinum(script,voxeltokptr)); break; }
#ifdef SUPERBUILD
                    for (tilex=tile0; tilex<=tile1; tilex++) tiletovox[tilex] = lastvoxid;
#endif
                    break; //last tile number (inclusive)
                case T_SCALE:
                {
                    double scale=1.0;
                    scriptfile_getdouble(script,&scale);
#ifdef SUPERBUILD
                    voxscale[lastvoxid] = (int32_t)(65536*scale);
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
            char *fn[6] = {0,0,0,0,0,0}, *modelend, happy=1, *tfn = NULL;
            int32_t i, tile = -1, pal = 0,ii;

            static const tokenlist skyboxtokens[] =
            {
                { "tile"   ,T_TILE   },
                { "pal"    ,T_PAL    },
                { "ft"     ,T_FRONT  },{ "front"  ,T_FRONT  },{ "forward",T_FRONT  },
                { "rt"     ,T_RIGHT  },{ "right"  ,T_RIGHT  },
                { "bk"     ,T_BACK   },{ "back"   ,T_BACK   },
                { "lf"     ,T_LEFT   },{ "left"   ,T_LEFT   },{ "lt"     ,T_LEFT   },
                { "up"     ,T_TOP    },{ "top"    ,T_TOP    },{ "ceiling",T_TOP    },{ "ceil"   ,T_TOP    },
                { "dn"     ,T_BOTTOM },{ "bottom" ,T_BOTTOM },{ "floor"  ,T_BOTTOM },{ "down"   ,T_BOTTOM }
            };

            if (scriptfile_getbraces(script,&modelend)) break;
            while (script->textptr < modelend)
            {
                switch (getatoken(script,skyboxtokens,sizeof(skyboxtokens)/sizeof(tokenlist)))
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
                }
            }

            if (tile < 0) initprintf("Error: missing 'tile number' for skybox definition near line %s:%d\n", script->filename, scriptfile_getlinum(script,skyboxtokptr)), happy=0;
            for (i=0; i<6; i++)
            {
                if (!fn[i]) initprintf("Error: missing '%s filename' for skybox definition near line %s:%d\n", skyfaces[i], script->filename, scriptfile_getlinum(script,skyboxtokptr)), happy = 0;
                ii = pathsearchmode;
                pathsearchmode = 1;
                if (findfrompath(fn[i],&tfn) < 0)
                {
                    char buf[BMAX_PATH];

                    Bstrcpy(buf,fn[i]);
                    kzfindfilestart(buf);
                    if (!kzfindfile(buf))
                    {
                        initprintf("Error: file '%s' does not exist\n",fn[i]);
                        happy = 0;
                    }
                }
                else Bfree(tfn);
                pathsearchmode = ii;
            }
            if (!happy) break;

            hicsetskybox(tile,pal,fn);
        }
        break;
        case T_TINT:
        {
            char *tinttokptr = script->ltextptr;
            int32_t red=255, green=255, blue=255, pal=-1, flags=0;
            char *tintend;

            static const tokenlist tinttokens[] =
            {
                { "pal",   T_PAL },
                { "red",   T_RED   },{ "r", T_RED },
                { "green", T_GREEN },{ "g", T_GREEN },
                { "blue",  T_BLUE  },{ "b", T_BLUE },
                { "flags", T_FLAGS }
            };

            if (scriptfile_getbraces(script,&tintend)) break;
            while (script->textptr < tintend)
            {
                switch (getatoken(script,tinttokens,sizeof(tinttokens)/sizeof(tokenlist)))
                {
                case T_PAL:
                    scriptfile_getsymbol(script,&pal);   break;
                case T_RED:
                    scriptfile_getnumber(script,&red);   red   = min(255,max(0,red));   break;
                case T_GREEN:
                    scriptfile_getnumber(script,&green); green = min(255,max(0,green)); break;
                case T_BLUE:
                    scriptfile_getnumber(script,&blue);  blue  = min(255,max(0,blue));  break;
                case T_FLAGS:
                    scriptfile_getsymbol(script,&flags); break;
                }
            }

            if (pal < 0)
            {
                initprintf("Error: missing 'palette number' for tint definition near line %s:%d\n", script->filename, scriptfile_getlinum(script,tinttokptr));
                break;
            }

            hicsetpalettetint(pal,red,green,blue,flags);
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
                token = getatoken(script,texturetokens,sizeof(texturetokens)/sizeof(tokenlist));
                switch (token)
                {
                case T_PAL:
                {
                    char *paltokptr = script->ltextptr, *palend;
                    int32_t pal=-1, i;
                    char *fn = NULL, *tfn = NULL;
                    double alphacut = -1.0, xscale = 1.0, yscale = 1.0, specpower = 1.0, specfactor = 1.0;
                    char flags = 0;

                    static const tokenlist texturetokens_pal[] =
                    {
                        { "file",            T_FILE },{ "name", T_FILE },
                        { "alphacut",        T_ALPHACUT },
                        { "detailscale",     T_XSCALE }, { "scale",  T_XSCALE }, { "xscale",  T_XSCALE }, { "intensity",  T_XSCALE },
                        { "yscale",          T_YSCALE },
                        { "specpower",       T_SPECPOWER }, { "parallaxscale", T_SPECPOWER },
                        { "specfactor",      T_SPECFACTOR }, { "parallaxbias", T_SPECFACTOR },
                        { "nocompress",      T_NOCOMPRESS },
                        { "nodownsize",      T_NODOWNSIZE },
                    };

                    if (scriptfile_getsymbol(script,&pal)) break;
                    if (scriptfile_getbraces(script,&palend)) break;
                    while (script->textptr < palend)
                    {
                        switch (getatoken(script,texturetokens_pal,sizeof(texturetokens_pal)/sizeof(tokenlist)))
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
                        case T_NOCOMPRESS:
                            flags |= 1; break;
                        case T_NODOWNSIZE:
                            flags |= 16; break;
                        default:
                            break;
                        }
                    }

                    if ((unsigned)tile > (unsigned)MAXTILES) break;	// message is printed later
                    if ((unsigned)pal >= ((unsigned)MAXPALOOKUPS - RESERVEDPALS))
                    {
                        initprintf("Error: missing or invalid 'palette number' for texture definition near "
                                   "line %s:%d\n", script->filename, scriptfile_getlinum(script,paltokptr));
                        break;
                    }
                    if (!fn)
                    {
                        initprintf("Error: missing 'file name' for texture definition near line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,paltokptr));
                        break;
                    }

                    i = pathsearchmode;
                    pathsearchmode = 1;
                    if (findfrompath(fn,&tfn) < 0)
                    {
                        char buf[BMAX_PATH];

                        Bstrcpy(buf,fn);
                        kzfindfilestart(buf);
                        if (!kzfindfile(buf))
                        {
                            initprintf("Error: file '%s' does not exist\n",fn);
                            pathsearchmode = i;
                            break;
                        }
                    }
                    else Bfree(tfn);
                    pathsearchmode = i;
                    xscale = 1.0f / xscale;
                    yscale = 1.0f / yscale;

                    hicsetsubsttex(tile,pal,fn,alphacut,xscale,yscale, specpower, specfactor,flags);
                }
                break;
                case T_DETAIL: case T_GLOW: case T_SPECULAR: case T_NORMAL:
                {
                    char *detailtokptr = script->ltextptr, *detailend;
                    int32_t pal = 0, i;
                    char *fn = NULL, *tfn = NULL;
                    double xscale = 1.0, yscale = 1.0, specpower = 1.0, specfactor = 1.0;
                    char flags = 0;

                    static const tokenlist texturetokens_pal[] =
                    {
                        { "file",            T_FILE },{ "name", T_FILE },
                        { "alphacut",        T_ALPHACUT },
                        { "detailscale",     T_XSCALE }, { "scale",  T_XSCALE }, { "xscale",  T_XSCALE }, { "intensity",  T_XSCALE },
                        { "yscale",          T_YSCALE },
                        { "specpower",       T_SPECPOWER }, { "parallaxscale", T_SPECPOWER },
                        { "specfactor",      T_SPECFACTOR }, { "parallaxbias", T_SPECFACTOR },
                        { "nocompress",      T_NOCOMPRESS },
                        { "nodownsize",      T_NODOWNSIZE },
                    };

                    if (scriptfile_getbraces(script,&detailend)) break;
                    while (script->textptr < detailend)
                    {
                        switch (getatoken(script,texturetokens_pal,sizeof(texturetokens_pal)/sizeof(tokenlist)))
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
                        case T_NOCOMPRESS:
                            flags |= 1; break;
                        case T_NODOWNSIZE:
                            flags |= 16; break;
                        default:
                            break;
                        }
                    }

                    if ((unsigned)tile > (unsigned)MAXTILES) break;	// message is printed later
                    if (!fn)
                    {
                        initprintf("Error: missing 'file name' for texture definition near line %s:%d\n",
                                   script->filename, scriptfile_getlinum(script,detailtokptr));
                        break;
                    }

                    i = pathsearchmode;
                    pathsearchmode = 1;
                    if (findfrompath(fn,&tfn) < 0)
                    {
                        char buf[BMAX_PATH];

                        Bstrcpy(buf,fn);
                        kzfindfilestart(buf);
                        if (!kzfindfile(buf))
                        {
                            initprintf("Error: file '%s' does not exist\n",fn);
                            pathsearchmode = i;
                            break;
                        }
                    }
                    else Bfree(tfn);
                    pathsearchmode = i;

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
                }
                break;
                default:
                    break;
                }
            }
            if ((unsigned)tile >= (unsigned)MAXTILES)
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

            if (scriptfile_getsymbol(script,&r0)) break;
            if (tokn == T_UNDEFMODELRANGE)
            {
                if (scriptfile_getsymbol(script,&r1)) break;
                if (r1 < r0)
                {
                    int32_t t = r1;
                    r1 = r0;
                    r0 = t;
                    initprintf("Warning: backwards tile range on line %s:%d\n", script->filename, scriptfile_getlinum(script,cmdtokptr));
                }
                if (r0 < 0 || r1 >= MAXTILES)
                {
                    initprintf("Error: invalid tile range on line %s:%d\n", script->filename, scriptfile_getlinum(script,cmdtokptr));
                    break;
                }
            }
            else
            {
                r1 = r0;
                if ((unsigned)r0 >= (unsigned)MAXTILES)
                {
                    initprintf("Error: invalid tile number on line %s:%d\n", script->filename, scriptfile_getlinum(script,cmdtokptr));
                    break;
                }
            }
#if defined(POLYMOST) && defined(USE_OPENGL)
            for (; r0 <= r1; r0++) md_undefinetile(r0);
#endif
        }
        break;

        case T_UNDEFMODELOF:
        {
            int32_t r0;
#if defined(POLYMOST) && defined(USE_OPENGL)
            int32_t mid;
#endif

            if (scriptfile_getsymbol(script,&r0)) break;
            if ((unsigned)r0 >= (unsigned)MAXTILES)
            {
                initprintf("Error: invalid tile number on line %s:%d\n", script->filename, scriptfile_getlinum(script,cmdtokptr));
                break;
            }

#if defined(POLYMOST) && defined(USE_OPENGL)
            mid = md_tilehasmodel(r0,0);
            if (mid < 0) break;

            md_undefinemodel(mid);
#endif
        }
        break;

        case T_UNDEFTEXTURE:
        case T_UNDEFTEXTURERANGE:
        {
            int32_t r0,r1,i;

            if (scriptfile_getsymbol(script,&r0)) break;
            if (tokn == T_UNDEFTEXTURERANGE)
            {
                if (scriptfile_getsymbol(script,&r1)) break;
                if (r1 < r0)
                {
                    int32_t t = r1;
                    r1 = r0;
                    r0 = t;
                    initprintf("Warning: backwards tile range on line %s:%d\n", script->filename, scriptfile_getlinum(script,cmdtokptr));
                }
                if (r0 < 0 || r1 >= MAXTILES)
                {
                    initprintf("Error: invalid tile range on line %s:%d\n", script->filename, scriptfile_getlinum(script,cmdtokptr));
                    break;
                }
            }
            else
            {
                r1 = r0;
                if ((unsigned)r0 >= (unsigned)MAXTILES)
                {
                    initprintf("Error: invalid tile number on line %s:%d\n", script->filename, scriptfile_getlinum(script,cmdtokptr));
                    break;
                }
            }

            for (; r0 <= r1; r0++)
                for (i=MAXPALOOKUPS-1; i>=0; i--)
                    hicclearsubst(r0,i);
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

            if (scriptfile_getbraces(script,&dummy)) break;
            while (script->textptr < dummy)
            {
                switch (getatoken(script,sound_musictokens,sizeof(sound_musictokens)/sizeof(tokenlist)))
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

        default:
            initprintf("Unknown token.\n"); break;
        }
    }
    return 0;
}


int32_t loaddefinitionsfile(char *fn)
{
    scriptfile *script;
    int32_t f = flushlogwindow;

    script = scriptfile_fromfile(fn);
    if (!script) return -1;

    flushlogwindow = 1;
    initprintf("Loading '%s'\n",fn);
    flushlogwindow = 0;
    defsparser(script);

    flushlogwindow = f;
    scriptfile_close(script);
    scriptfile_clearsymbols();

    return 0;
}

// vim:ts=4:
