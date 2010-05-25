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

#include "m32script.h"
#include "m32def.h"
#include "cache1d.h"
#include "sounds_mapster32.h"

//#include "osd.h"
#include "keys.h"

char g_szScriptFileName[BMAX_PATH] = "(none)";  // file we're currently compiling
static char g_szCurrentBlockName[256] = "(none)", g_szLastBlockName[256] = "NULL";

////// compiler state vvv
static const char *textptr;
int32_t g_totalLines, g_lineNumber;
int32_t g_numCompilerErrors, g_numCompilerWarnings;

int32_t g_didDefineSomething;

typedef struct
{
    int32_t currentStateIdx;
    ofstype currentStateOfs;  // the offset to the start of the currently parsed states' code
    int32_t currentEvent;
    ofstype parsingEventOfs;

    int32_t checkingSwitch;
    int32_t numCases;
    instype *caseScriptPtr;  // the pointer to the start of the case table in a switch statement
    // first entry is 'default' code.
    instype *caseCodePtr;  // the pointer to the start of the different cases' code
    int32_t labelsOnly;
    int32_t numBraces;
    int32_t checkingIfElse, ifElseAborted;
} compilerstate_t;

static compilerstate_t cs;
static compilerstate_t cs_default = {-1, -1, -1, -1, 0, 0, NULL, NULL, 0, 0, 0, 0};
////// -------------------

instype *script = NULL;
instype *g_scriptPtr;
int32_t g_scriptSize = 65536;

int32_t *constants, constants_allocsize=1024;
int32_t g_numSavedConstants=0;
static int32_t g_wasConstant=0;

char *label;
int32_t *labelval;
uint8_t *labeltype;
int32_t g_numLabels=0, g_numDefaultLabels=0;
static int32_t label_allocsize = 512;

int32_t g_stateCount = 0;
statesinfo_t *statesinfo = NULL;
static int32_t statesinfo_allocsize = 512;

static char tempbuf[2048];
static char tlabel[MAXLABELLEN];

int32_t g_iReturnVar=0;
int32_t m32_sortvar1, m32_sortvar2;

char *ScriptQuotes[MAXQUOTES+1], *ScriptQuoteRedefinitions[MAXQUOTES+1];
int32_t g_numQuoteRedefinitions = 0;

ofstype aEventOffsets[MAXEVENTS];
int32_t aEventSizes[MAXEVENTS];

gamevar_t aGameVars[MAXGAMEVARS];
gamearray_t aGameArrays[MAXGAMEARRAYS];
int32_t g_gameVarCount=0, g_systemVarCount=0;
int32_t g_gameArrayCount=0, g_systemArrayCount=0;


// "magic" number for { and }, overrides line number in compiled code for later detection
#define IFELSE_MAGIC 31337


void C_ReportError(int32_t iError);

enum ScriptLabel_t
{
    LABEL_ANY    = -1,
    LABEL_DEFINE = 1,
    LABEL_EVENT  = 2
};

static const char *LabelTypeText[] =
{
    "define",
    "event"
};

static const char *C_GetLabelType(int32_t type)
{
    uint32_t i;
    char x[64];

    x[0] = 0;
    for (i=0; i<sizeof(LabelTypeText)/sizeof(char*); i++)
    {
        if (!(type & (1<<i))) continue;
        if (x[0]) Bstrcat(x, " or ");
        Bstrcat(x, LabelTypeText[i]);
    }
    return strdup(x);
}

#define NUMKEYWORDS (int32_t)(sizeof(keyw)/sizeof(keyw[0]))
#define NUMALTKEYWORDS (int32_t)(sizeof(altkeyw)/sizeof(altkeyw[0]))

const tokenmap_t altkeyw[] =
{
    { "#define", CON_DEFINE },
    { "al", CON_ADDLOGVAR },
    { "var", CON_GAMEVAR },
    { "array", CON_GAMEARRAY },
    { "shiftl", CON_SHIFTVARL },
    { "shiftr", CON_SHIFTVARR },
    { "rand", CON_RANDVARVAR },
    { "set", CON_SETVARVAR },
    { "add", CON_ADDVARVAR },
    { "sub", CON_SUBVARVAR },
    { "mul", CON_MULVARVAR },
    { "div", CON_DIVVARVAR },
    { "mod", CON_MODVARVAR },
    { "and", CON_ANDVARVAR },
    { "or", CON_ORVARVAR },
    { "xor", CON_XORVARVAR },
    { "ifl", CON_IFVARVARL },
    { "ifle", CON_IFVARVARLE },
    { "ifg", CON_IFVARVARG },
    { "ifge", CON_IFVARVARGE },
    { "ife", CON_IFVARVARE },
    { "ifn", CON_IFVARVARN },
    { "ifand", CON_IFVARVARAND },
    { "ifor", CON_IFVARVAROR },
    { "ifxor", CON_IFVARVARXOR },
    { "ifeither", CON_IFVARVAREITHER },
    { "ifboth", CON_IFVARVARBOTH },
    { "whilen", CON_WHILEVARVARN },
    { "whilel", CON_WHILEVARVARL },
};

const char *keyw[] =
{
    "nullop",
    "define",
    "include",
    "defstate",  // *
    "ends",
    "state",
    "onevent",
    "endevent",
    "gamevar",

    "else",
    "return",
    "break",
    "switch",
    "case",
    "default",
    "endswitch",
    "getcurraddress",
    "jump",
    "{",
    "}",

    "setsector",
    "getsector",
    "setwall",
    "getwall",
    "setsprite",
    "getsprite",
    "gettspr",
    "settspr",

    "gamearray",
    "setarray",
    "getarraysize",
    "resizearray",
    "copy",

    "randvar",
    "displayrandvar",
    "setvar",
    "addvar",
    "subvar",
    "mulvar",
    "divvar",
    "modvar",
    "andvar",
    "orvar",
    "xorvar",
    "shiftvarl",
    "shiftvarr",

    "randvarvar",
    "displayrandvarvar",
    "setvarvar",  // *
    "addvarvar",
    "subvarvar",
    "mulvarvar",
    "divvarvar",
    "modvarvar",
    "andvarvar",
    "orvarvar",
    "xorvarvar",
    "sin",
    "cos",

    "displayrand",

    "itof",
    "ftoi",
    "clamp",
    "inv",  //  inversion function.. not internal
    "sqrt",
    "mulscale",
    "dist",
    "ldist",
    "getangle",
    "getincangle",

    "sort",
    "for",  // *

    "ifvarl",
    "ifvarle",
    "ifvarg",
    "ifvarge",
    "ifvare",
    "ifvarn",
    "ifvarand",
    "ifvaror",
    "ifvarxor",
    "ifvareither",
    "ifvarboth",
    "whilevarn",
    "whilevarl",

    "ifvarvarl",
    "ifvarvarle",
    "ifvarvarg",
    "ifvarvarge",
    "ifvarvare",
    "ifvarvarn",
    "ifvarvarand",
    "ifvarvaror",
    "ifvarvarxor",
    "ifvarvareither",
    "ifvarvarboth",
    "whilevarvarn",
    "whilevarvarl",

    "ifhitkey",
    "ifholdkey",
    "ifrnd",
    "ifangdiffl",
    "ifspritepal",
    "ifactor",
    "ifsound",
    "ifpdistl",
    "ifpdistg",

    "ifinside",

    "ifeitheralt",
    "ifeitherctrl",
    "ifeithershift",
    "ifawayfromwall",
    "ifcansee",
    "ifonwater",
    "ifinwater",
    "ifoutside",
    "ifnosounds",

// BUILD functions
    "resetkey",
    "insertsprite",
    "dupsprite",
    "tdupsprite",
    "deletesprite",
    "lastwall",
    "updatecursectnum",
    "updatesector",
    "updatesectorz",
    "getzrange",
    "hitscan",
    "cansee",
    "canseespr",
    "neartag",
    "rotatepoint",
    "dragpoint",
    "getceilzofslope",
    "getflorzofslope",
    "alignceilslope",
    "alignflorslope",
    "bsetsprite",  // *
    "setfirstwall",
    "changespritestat",
    "changespritesect",
    "headspritestat",
    "prevspritestat",
    "nextspritestat",
    "headspritesect",
    "prevspritesect",
    "nextspritesect",
    "sectorofwall",
    "fixrepeats",

    "addlogvar",
    "addlog",
    "debug",

    "definequote",
    "redefinequote",
    "print",
    "quote",
    "error",
    "printmessage16",
    "printmessage256",
    "printext256",
    "printext16",
    "getnumber16",
    "getnumber256",
    "qsprintf",
    "qstrcat",
    "qstrcpy",
    "qstrlen",
//    "qgetsysstr",
    "qstrncat",
    "qsubstr",

    "findnearsprite",
    "findnearspritevar",
    "findnearsprite3d",
    "findnearsprite3dvar",
    "findnearspritez",
    "findnearspritezvar",

    "getticks",
    "gettimedate",
    "setaspect",

    "seti",
    "sizeat",
    "cstat",
    "cstator",
    "clipdist",
    "spritepal",
    "cactor",
    "spgetlotag",
    "spgethitag",
    "sectgetlotag",
    "sectgethitag",
    "gettexturefloor",
    "gettextureceiling",

    "sound", //var
    "soundonce", //var
    "stopallsounds",
    "stopsound", //var
    "globalsound", //var
    "getsoundflags",

///    "killit",

    "drawline16",
    "drawline16b",
    "drawcircle16",
    "drawcircle16b",
    "rotatesprite16",
    "rotatesprite",
    "setgamepalette",

    "<null>"
};

const memberlabel_t SectorLabels[]=
{
    { "wallptr", SECTOR_WALLPTR, 1, 0, 0 },
    { "wallnum", SECTOR_WALLNUM, 1, 0, 0 },
    { "ceilingz", SECTOR_CEILINGZ, 0, 0, 0 },
    { "floorz", SECTOR_FLOORZ, 0, 0, 0 },
    { "ceilingstat", SECTOR_CEILINGSTAT, 0, 0, 0 },
    { "floorstat", SECTOR_FLOORSTAT, 0, 0, 0 },
    { "ceilingpicnum", SECTOR_CEILINGPICNUM, 0, 0, MAXTILES-1 },
    { "ceilingslope", SECTOR_CEILINGSLOPE, 0, 0, 0},
    { "ceilingshade", SECTOR_CEILINGSHADE, 0, 0, 0 },
    { "ceilingpal", SECTOR_CEILINGPAL, 0, 0, 0 },
    { "ceilingxpanning", SECTOR_CEILINGXPANNING, 0, 0, 0 },
    { "ceilingypanning", SECTOR_CEILINGYPANNING, 0, 0, 0 },
    { "floorpicnum", SECTOR_FLOORPICNUM, 0, 0, MAXTILES-1 },
    { "floorslope", SECTOR_FLOORSLOPE, 0, 0, 0 },
    { "floorshade", SECTOR_FLOORSHADE, 0, 0, 0 },
    { "floorpal", SECTOR_FLOORPAL, 0, 0, 0 },
    { "floorxpanning", SECTOR_FLOORXPANNING, 0, 0, 0 },
    { "floorypanning", SECTOR_FLOORYPANNING, 0, 0, 0 },
    { "visibility", SECTOR_VISIBILITY, 0, 0, 0 },
    { "alignto", SECTOR_ALIGNTO, 0, 0, 0 }, // aka filler, not used
    { "lotag", SECTOR_LOTAG, 0, 0, 0 },
    { "hitag", SECTOR_HITAG, 0, 0, 0 },
    { "extra", SECTOR_EXTRA, 0, 0, 0 },
    { "", -1, 0, 0, 0  }     // END OF LIST
};

const memberlabel_t WallLabels[]=
{
    { "x", WALL_X, 0, -524288, 524288 },
    { "y", WALL_Y, 0, -524288, 524288 },
    { "point2", WALL_POINT2, 1, 0, 0 },
    { "nextwall", WALL_NEXTWALL, 1, 0, 0 },
    { "nextsector", WALL_NEXTSECTOR, 1, 0, 0 },
    { "cstat", WALL_CSTAT, 0, 0, 0 },
    { "picnum", WALL_PICNUM, 0, 0, MAXTILES-1 },
    { "overpicnum", WALL_OVERPICNUM, 0, 0, MAXTILES-1 },
    { "shade", WALL_SHADE, 0, 0, 0 },
    { "pal", WALL_PAL, 0, 0, 0 },
    { "xrepeat", WALL_XREPEAT, 0, 0, 0 },
    { "yrepeat", WALL_YREPEAT, 0, 0, 0 },
    { "xpanning", WALL_XPANNING, 0, 0, 0 },
    { "ypanning", WALL_YPANNING, 0, 0, 0 },
    { "lotag", WALL_LOTAG, 0, 0, 0 },
    { "hitag", WALL_HITAG, 0, 0, 0 },
    { "extra", WALL_EXTRA, 0, 0, 0 },
    { "", -1, 0, 0, 0  }     // END OF LIST
};

const memberlabel_t SpriteLabels[]=
{
    { "x", SPRITE_X, 0, -524288, 524288 },
    { "y", SPRITE_Y, 0, -524288, 524288 },
    { "z", SPRITE_Z, 0, 0, 0 },
    { "cstat", SPRITE_CSTAT, 0, 0, 0 },
    { "picnum", SPRITE_PICNUM, 0, 0, MAXTILES-1 },
    { "shade", SPRITE_SHADE, 0, 0, 0 },
    { "pal", SPRITE_PAL, 0, 0, 0 },
    { "clipdist", SPRITE_CLIPDIST, 0, 0, 0 },
    { "detail", SPRITE_DETAIL, 1, 0, 0 }, // aka filler, not used
    { "xrepeat", SPRITE_XREPEAT, 0, 0, 0 },
    { "yrepeat", SPRITE_YREPEAT, 0, 0, 0 },
    { "xoffset", SPRITE_XOFFSET, 0, 0, 0 },
    { "yoffset", SPRITE_YOFFSET, 0, 0, 0 },
    { "sectnum", SPRITE_SECTNUM, 1, 0, 0 },
    { "statnum", SPRITE_STATNUM, 1, 0, 0 },
    { "ang", SPRITE_ANG, 0, 0, 0 },
    { "owner", SPRITE_OWNER, 0, 0, 0 },
    { "xvel", SPRITE_XVEL, 0, 0, 0 },
    { "yvel", SPRITE_YVEL, 0, 0, 0 },
    { "zvel", SPRITE_ZVEL, 0, 0, 0 },
    { "lotag", SPRITE_LOTAG, 0, 0, 0 },
    { "hitag", SPRITE_HITAG, 0, 0, 0 },
    { "extra", SPRITE_EXTRA, 0, 0, 0 },
    { "", -1, 0, 0, 0 }     // END OF LIST
};

const tokenmap_t iter_tokens[] =
{
    { "allsprites", ITER_ALLSPRITES },
    { "allsectors", ITER_ALLSECTORS },
    { "allwalls", ITER_ALLWALLS },
    { "selsprites", ITER_SELSPRITES },
    { "selsectors", ITER_SELSECTORS },
    { "selwalls", ITER_SELWALLS },
    { "drawnsprites", ITER_DRAWNSPRITES },
    { "spritesofsector", ITER_SPRITESOFSECTOR },
    { "loopofwall", ITER_LOOPOFWALL },
    { "wallsofsector", ITER_WALLSOFSECTOR },
    { "range", ITER_RANGE },
// vvv alternatives go here vvv
    { "selspr", ITER_SELSPRITES },
    { "selsec", ITER_SELSECTORS },
    { "sprofsec", ITER_SPRITESOFSECTOR },
    { "walofsec", ITER_WALLSOFSECTOR },
    { "", -1 }     // END OF LIST
};


hashtable_t h_gamevars = { MAXGAMEVARS>>1, NULL };
hashtable_t h_arrays   = { MAXGAMEARRAYS>>1, NULL };
hashtable_t h_labels   = { 11262>>1, NULL };

static hashtable_t h_states   = { 1264>>1, NULL };
static hashtable_t h_keywords = { CON_END>>1, NULL };
static hashtable_t h_iter    = { ITER_END, NULL };

static hashtable_t h_sector  = { SECTOR_END>>1, NULL };
static hashtable_t h_wall    = { WALL_END>>1, NULL };
static hashtable_t h_sprite  = { SPRITE_END>>1, NULL };


static void C_InitHashes()
{
    int32_t i;

    hash_init(&h_gamevars);
    hash_init(&h_arrays);
    hash_init(&h_labels);
    hash_init(&h_states);

    hash_init(&h_keywords);
    for (i=NUMKEYWORDS-1; i>=0; i--)
        hash_add(&h_keywords, keyw[i], i);
    for (i=0; i<NUMALTKEYWORDS; i++)
        hash_add(&h_keywords, altkeyw[i].token, altkeyw[i].val);

    hash_init(&h_sector);
    for (i=0; SectorLabels[i].lId >=0; i++)
        hash_add(&h_sector,SectorLabels[i].name,i);
    hash_add(&h_sector,"filler", SECTOR_ALIGNTO);

    hash_init(&h_wall);
    for (i=0; WallLabels[i].lId >=0; i++)
        hash_add(&h_wall,WallLabels[i].name,i);

    hash_init(&h_sprite);
    for (i=0; SpriteLabels[i].lId >=0; i++)
        hash_add(&h_sprite,SpriteLabels[i].name,i);
    hash_add(&h_sprite,"filler", SPRITE_DETAIL);

    hash_init(&h_iter);
    for (i=0; iter_tokens[i].val >=0; i++)
        hash_add(&h_iter, iter_tokens[i].token, iter_tokens[i].val);
}

static int32_t C_SetScriptSize(int32_t size)
{
    ofstype oscriptOfs = (unsigned)(g_scriptPtr-script);
    ofstype ocaseScriptOfs = (unsigned)(cs.caseScriptPtr-script);
    ofstype ocaseCodeOfs = (unsigned)(cs.caseCodePtr-script);

    instype *newscript;
    int32_t osize = g_scriptSize;

    if (g_scriptSize >= size)
        return 0;

    //initprintf("offset: %d\n",(unsigned)(g_scriptPtr-script));
    g_scriptSize = size;
    initprintf("Resizing code buffer to %d*%d bytes\n", g_scriptSize, sizeof(instype));

    newscript = (instype *)Brealloc(script, g_scriptSize * sizeof(instype));

    if (newscript == NULL)
    {
        C_ReportError(-1);
        initprintf("%s:%d: out of memory: Aborted (%ud)\n", g_szScriptFileName, g_lineNumber, (unsigned)(g_scriptPtr-script));
        g_numCompilerErrors++;
//        initprintf(tempbuf);
        return 1;
    }

    if (size >= osize)
        Bmemset(&newscript[osize], 0, (size-osize) * sizeof(instype));

    if (script != newscript)
    {
        initprintf("Relocating compiled code from to 0x%x to 0x%x\n", script, newscript);
        script = newscript;
    }

    g_scriptPtr = (instype *)(script+oscriptOfs);
//    initprintf("script: %d, \n",script); initprintf("offset: %d\n",(unsigned)(g_scriptPtr-script));

    if (cs.caseScriptPtr != NULL)
        cs.caseScriptPtr = (instype *)(script+ocaseScriptOfs);
    if (cs.caseCodePtr != NULL)
        cs.caseCodePtr = (instype *)(script+ocaseCodeOfs);

    return 0;
}

static inline int32_t ispecial(char c)
{
    return c==' ' || c=='\t' || c=='\n' || c=='\r' || c=='(' || c==')' || c==',' || c==';';
}

static inline int32_t isaltok(char c)
{
    return isalnum(c) || c == '#' || c == '{' || c == '}' || c == '/' || c == '\\' ||
                         c == '*' || c == '-' || c == '_' || c == '.';
}

static int32_t C_SkipComments(void)
{
    char c = *textptr;

    do
    {
        if (ispecial(c))
        {
            textptr++;
            g_lineNumber += (c=='\n');
        }
        else if (c == '/' && textptr[1] == '/')
        {
            while (*textptr && *textptr != 0x0a && *textptr != 0x0d)
                textptr++;
        }
        else if (c == '/' && textptr[1] == '*')
        {
            textptr += 2;

            while (*textptr && !(textptr[0] == '*' && textptr[1] == '/'))
            {
                if (*textptr == '\n')
                    g_lineNumber++;
                textptr++;
            }

            if (!*textptr)
            {
                C_CUSTOMERROR("found `/*' with no `*/'.");
                cs.numBraces = 0;
                cs.currentStateIdx = -1;
                break;
            }

            textptr += 2;
        }
        else
            break;
    }
    while ((c = *textptr));

    if ((unsigned)(g_scriptPtr-script) > (unsigned)(g_scriptSize-32))
        return C_SetScriptSize(g_scriptSize<<1);

    return 0;
}

static inline int32_t C_GetLabelNameID(const memberlabel_t *pLabel, hashtable_t *tH, const char *psz)
{
    // find the label psz in the table pLabel.
    // returns the ID for the label, or -1

    int32_t l = hash_findcase(tH, psz);
    if (l>=0)
        l = pLabel[l].lId;

    return l;
}

static void C_GetNextLabelName(void)
{
    int32_t i;

    C_SkipComments();

    if (*textptr == 0)
    {
        C_CUSTOMERROR("unexpected EOF where label was expected.");
        return;
    }

    while (!isalnum(*textptr))
    {
        g_lineNumber += (*textptr == 0x0a);

        textptr++;
        if (!*textptr)
            return;
    }

    i = 0;
    while (!ispecial(*textptr) && *textptr!='['&& *textptr!=']')
    {
        if (i < MAXLABELLEN-1)
            tlabel[i++] = *(textptr++);
        else
            textptr++;
    }

    tlabel[i] = 0;
//    if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug > 1)
//        initprintf("%s:%d: debug: got label `%s'.\n",g_szScriptFileName,g_lineNumber,tlabel);
}

static int32_t C_CopyLabel(void)
{
    if (g_numLabels >= label_allocsize)
    {
        label = Brealloc(label, 2*label_allocsize*MAXLABELLEN*sizeof(char));
        labelval = Brealloc(labelval, 2*label_allocsize*sizeof(labelval[0]));
        labeltype = Brealloc(labeltype, 2*label_allocsize*sizeof(labeltype[0]));

        if (label==NULL || labelval==NULL || labeltype==NULL)
        {
            label_allocsize = 0;
            initprintf("Out of memory!");
            return 1;
        }
        label_allocsize *= 2;
    }

    Bmemcpy(label+(g_numLabels*MAXLABELLEN), tlabel, MAXLABELLEN);
    return 0;
}

static int32_t C_GetKeyword(void)
{
    int32_t i;
    const char *temptextptr;

    C_SkipComments();

    if (*textptr == 0)
        return -1;

    temptextptr = textptr;

    while (!isaltok(*temptextptr))
    {
        temptextptr++;
        if (!*temptextptr)
            return 0;
    }

    i = 0;
    while (isaltok(*temptextptr))
        tempbuf[i++] = *(temptextptr++);
    tempbuf[i] = 0;

    return hash_find(&h_keywords, tempbuf);
}

static int32_t C_GetNextKeyword(void)  //Returns its code #
{
    int32_t i, l;

    C_SkipComments();

    if (*textptr == 0)
        return -1;

    while (!isaltok(*textptr))
    {
        g_lineNumber += (*textptr == 0x0a);

        if (!*textptr)
            return -1;

        textptr++;
    }

    l = 0;
    while (isaltok(*(textptr+l)) && !(*(textptr + l) == '.'))
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    while (isaltok(*(textptr+l)))
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    i = hash_find(&h_keywords, tempbuf);
    if (i>=0)
    {
        if (i == CON_LEFTBRACE || i == CON_RIGHTBRACE || i == CON_NULLOP)
            *g_scriptPtr = i + (IFELSE_MAGIC<<12);
        else
            *g_scriptPtr = i + (g_lineNumber<<12);

        textptr += l;
        g_scriptPtr++;

//        if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug)
//            initprintf("%s:%d: debug: translating keyword `%s'.\n",g_szScriptFileName,g_lineNumber,keyw[i]);
        return i;
    }

    textptr += l;

    if (tempbuf[0] == '{' && tempbuf[1] != 0)
    {
        C_CUSTOMERROR("expected whitespace between `{' and `%s'", tempbuf+1);
    }
    else if (tempbuf[0] == '}' && tempbuf[1] != 0)
    {
        C_CUSTOMERROR("expected whitespace between `}' and `%s'", tempbuf+1);
    }
    else
    {
        C_ReportError(ERROR_EXPECTEDKEYWORD);
        g_numCompilerErrors++;
    }

    return -1;
}

#define GetGamevarID(szGameLabel) hash_find(&h_gamevars, szGameLabel)
#define GetGamearrayID(szGameLabel) hash_find(&h_arrays, szGameLabel)

static void C_GetNextVarType(int32_t type)
{
    int32_t i, id=0, flags=0, num, indirect=0;

    C_SkipComments();

    if (*textptr == 0)
    {
        C_CUSTOMERROR("unexpected EOF where variable was expected.");
        return;
    }

    g_wasConstant = 0;

    // constant where gamevar expected
    if ((type==0 || type==GAMEVAR_SPECIAL) && !cs.labelsOnly &&
            (isdigit(textptr[0]) || (textptr[0]=='-' && isdigit(textptr[1]))))
    {
//        if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug)
//            initprintf("%s:%d: debug: accepted constant %d in place of gamevar.\n",g_szScriptFileName,g_lineNumber,atol(textptr));

        if (tolower(textptr[1])=='x')
            sscanf(textptr+2, "%" SCNx32, &num);
        else
            num = atoi(textptr);

        if (type==GAMEVAR_SPECIAL && (num<0 || num>=65536))
            C_CUSTOMERROR("array index %d out of bounds. (max: 65535)",  num);

        if (g_numCompilerErrors==0 && type!=GAMEVAR_SPECIAL && num != (int16_t)num)
        {
            indirect = 1;

            for (i=g_numSavedConstants-1; i>=0; i--)
                if (constants[i] == num)
                    break;

            if (i<0)
            {
                i = g_numSavedConstants;
                if (i>=constants_allocsize)
                {
                    constants_allocsize *= 2;
                    constants = Brealloc(constants, constants_allocsize * sizeof(constants[0]));
                    if (!constants)
                    {
                        initprintf("C_GetNextVarType(): ERROR: out of memory!\n");
                        g_numCompilerErrors++;
                        return;
                    }
                }

                constants[i] = num;
                num = i;
                g_numSavedConstants++;
            }
        }

        if (type!=GAMEVAR_SPECIAL)
            g_wasConstant = 1;

        *g_scriptPtr++ = MAXGAMEVARS | (num<<16) | indirect;

        while (!ispecial(*textptr) && *textptr != ']')
            textptr++;

        return;
    }
    else if (textptr[0]=='-' && textptr[1] != '.') //  && !isdigit(*(textptr+1))
    {
        if (type==0)
        {
//            if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug)
//                initprintf("%s:%d: debug: flagging gamevar as negative.\n",g_szScriptFileName,g_lineNumber,atol(textptr));
            flags = M32_FLAG_NEGATE;
        }
        else
        {
            C_CUSTOMERROR("error: syntax error. Tried negating written-to variable.");
            C_GetNextLabelName();
            return;
        }
    }
    else if (type != GAMEVAR_SPECIAL &&
                 (textptr[0]=='.' || (textptr[0]=='-' && textptr[1]=='.')))
    {
        int32_t lLabelID = -1, aridx = M32_THISACTOR_VAR_ID;

        flags |= M32_FLAG_SPECIAL;

        if (*textptr=='-')
        {
            flags |= M32_FLAG_NEGATE;
            textptr++;
        }
        textptr++;

        /// now pointing at 'xxx'

        C_GetNextLabelName();
        lLabelID = C_GetLabelNameID(SpriteLabels, &h_sprite, Bstrtolower(tlabel));

        if (lLabelID == -1)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return;
        }

        id = M32_SPRITE_VAR_ID;
        *g_scriptPtr++ = ((aridx<<16) | id | (lLabelID<<2) | flags);

        return;
    }

    C_GetNextLabelName();
    if (hash_find(&h_keywords, tlabel)>=0)
    {
        g_numCompilerErrors++;
        C_ReportError(ERROR_ISAKEYWORD);
        return;
    }

    C_SkipComments();  //skip comments and whitespace
    if (*textptr == '[')  //read of array as a gamevar
    {
        int32_t lLabelID = -1, aridx;

        textptr++;
        flags |= M32_FLAG_ARRAY;

        if (type & GAMEVAR_SPECIAL)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_EXPECTEDSIMPLEVAR);
            return;
        }

        id = GetGamearrayID(tlabel);
        if (id < 0)
        {
            id = GetGamevarID(tlabel);
            if (id >= 4)
                id = -1;

            if (id < 0)
            {
                g_numCompilerErrors++;
                C_ReportError(ERROR_NOTAGAMEARRAY);
                return;
            }

            flags &= ~M32_FLAG_ARRAY; // not an array
            flags |= M32_FLAG_SPECIAL;
        }
        else
        {
            if ((aGameArrays[id].dwFlags & GAMEARRAY_READONLY) && (type&GAMEVAR_READONLY))
            {
                C_ReportError(ERROR_ARRAYREADONLY);
                g_numCompilerErrors++;
            }
        }

        C_GetNextVarType(GAMEVAR_SPECIAL);  // _SPECIAL signifies that we want only simple vars or a constant
        g_scriptPtr--;
        aridx = *g_scriptPtr;

        C_SkipComments();

        if (*textptr != ']')
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_GAMEARRAYBNC);
            return;
        }
        textptr++;

        // writing arrays in this way is not supported because it would
        // require too many changes to other code
//        if (type)
//        {
//            g_numCompilerErrors++;
//            C_ReportError(ERROR_INVALIDARRAYWRITE);
//            return;
//        }

        if (flags & M32_FLAG_SPECIAL)
        {
            while (*textptr && *textptr != 0x0a && *textptr != '.')
                textptr++;

            if (*textptr != '.')
            {
                static const char *types[4] = {"sprite","sector","wall","tsprite"};
                C_CUSTOMERROR("error: syntax error. Expected `.<label>' after `%s[...]'", types[id&3]);
                return;
            }
            textptr++;

            /// now pointing at 'xxx'

            C_GetNextLabelName();

            /*initprintf("found xxx label of '%s'\n",   label+(g_numLabels*MAXLABELLEN));*/
            if (id==M32_SPRITE_VAR_ID || id==M32_TSPRITE_VAR_ID)
                lLabelID = C_GetLabelNameID(SpriteLabels, &h_sprite, Bstrtolower(tlabel));
            else if (id==M32_SECTOR_VAR_ID)
                lLabelID = C_GetLabelNameID(SectorLabels, &h_sector, Bstrtolower(tlabel));
            else if (id==M32_WALL_VAR_ID)
                lLabelID = C_GetLabelNameID(WallLabels, &h_wall, Bstrtolower(tlabel));
            //printf("LabelID is %d\n",lLabelID);

            if (lLabelID == -1)
            {
                g_numCompilerErrors++;
                C_ReportError(ERROR_SYMBOLNOTRECOGNIZED);
                return;
            }

            if ((aridx & 0x0000FFFF) == MAXGAMEVARS)  // constant
                *g_scriptPtr++ = (aridx | id | (lLabelID<<2) | flags);
            else  // simple gamevar
                *g_scriptPtr++ = (aridx<<16 | id | (lLabelID<<2) | flags);
        }
        else // if (flags & M32_FLAG_ARRAY)
        {
            if ((aridx & 0x0000FFFF) == MAXGAMEVARS)  // constant
                *g_scriptPtr++ = (aridx | id | flags);
            else  // simple gamevar
                *g_scriptPtr++ = (aridx<<16 | id | flags);
        }
        return;
    }

//    initprintf("not an array");
    id = GetGamevarID(tlabel);
    if (id < 0)   //gamevar not found
    {
        if (!type && !cs.labelsOnly)
        {
            //try looking for a define instead
            id = hash_find(&h_labels, tlabel);
            if (id>=0 && labeltype[id]==LABEL_DEFINE)
            {
//                if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug)
//                    initprintf("%s:%d: debug: accepted defined label `%s' instead of gamevar.\n",g_szScriptFileName,g_lineNumber,label+(id*MAXLABELLEN));
                num = labelval[id];

                if (type==GAMEVAR_SPECIAL && (num<0 || num>=65536))
                {
                    C_CUSTOMERROR("label %s=%d not suitable as array index. (max: 65535)", label+(id*MAXLABELLEN), num);
                }
                else if (num != (int16_t)num)
                {
                    indirect = 2;
                    num = id;
                }

                if (type!=GAMEVAR_SPECIAL)
                    g_wasConstant = 1;

                *g_scriptPtr++ = MAXGAMEVARS | (num<<16) | indirect;
                return;
            }
        }

        g_numCompilerErrors++;
        C_ReportError(ERROR_NOTAGAMEVAR);
        textptr++;
        return;
    }

    if (type == GAMEVAR_READONLY && aGameVars[id].dwFlags & GAMEVAR_READONLY)
    {
        g_numCompilerErrors++;
        C_ReportError(ERROR_VARREADONLY);
        return;
    }
    else if (aGameVars[id].dwFlags & type)
    {
        g_numCompilerErrors++;
        C_ReportError(ERROR_VARTYPEMISMATCH);
        return;
    }
//    if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug > 1)
//        initprintf("%s:%d: debug: accepted gamevar `%s'.\n",g_szScriptFileName,g_lineNumber,tlabel);

    *g_scriptPtr++ = (id|flags);
}

static inline void C_GetNextVar()
{
    C_GetNextVarType(0);
}

static inline void C_GetManyVarsType(int32_t type, int32_t num)
{
    int32_t i;
    for (i=num-1; i>=0; i--)
        C_GetNextVarType(type);
}

static inline void C_GetManyVars(int32_t num)
{
    C_GetManyVarsType(0,num);
}

static int32_t C_GetNextValue(int32_t type)
{
    int32_t i, l;

    C_SkipComments();

    if (*textptr == 0)
    {
        C_CUSTOMERROR("unexpected EOF where value was expected.");
        return -1;
    }

    while (!isaltok(*textptr))
    {
        g_lineNumber += (*textptr == 0x0a);

        textptr++;

        if (!*textptr)
            return -1; // eof
    }

    l = 0;
    while (isaltok(*(textptr+l)))
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    if (hash_find(&h_keywords, tempbuf)>=0)
    {
        g_numCompilerErrors++;
        C_ReportError(ERROR_ISAKEYWORD);
        textptr+=l;
    }

    i = hash_find(&h_labels, tempbuf);
    if (i >= 0)
    {
        char *el,*gl;

        if (labeltype[i] & type)
        {
//            if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug > 1)
//            {
//                gl = (char *)C_GetLabelType(labeltype[i]);
//                initprintf("%s:%d: debug: accepted %s label `%s'.\n",g_szScriptFileName,g_lineNumber,gl,label+(i*MAXLABELLEN));
//                Bfree(gl);
//            }

            *(g_scriptPtr++) = labelval[i];

            textptr += l;
            return labeltype[i];
        }

        *(g_scriptPtr++) = 0;
        textptr += l;

        el = (char *)C_GetLabelType(type);
        gl = (char *)C_GetLabelType(labeltype[i]);
        C_CUSTOMERROR("expected %s, found %s.", el, gl);
//        initprintf("i=%d, %s!!! lt:%d t:%d\n", i, label+(i*MAXLABELLEN), labeltype[i], type);
        Bfree(el);
        Bfree(gl);

        return -1;  // valid label name, but wrong type
    }

    if (isdigit(*textptr) == 0 && *textptr != '-')
    {
        C_ReportError(ERROR_PARAMUNDEFINED);
        g_numCompilerErrors++;

        *g_scriptPtr = 0;
        g_scriptPtr++;

        textptr += l;
        return -1; // error!
    }

    if (isdigit(*textptr) && cs.labelsOnly)
    {
        C_ReportError(WARNING_LABELSONLY);
        g_numCompilerWarnings++;
    }

    i = l-1;
    do
    {
        // FIXME: check for 0-9 A-F for hex
        if (textptr[0] == '0' && textptr[1] == 'x') break; // kill the warning for hex
        if (!isdigit(textptr[i--]))
        {
            C_CUSTOMWARNING("invalid character `%c' in definition!", textptr[i+1]);
            break;
        }
    }
    while (i > 0);

//    if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug > 1)
//        initprintf("%s:%d: debug: accepted constant %d.\n",g_szScriptFileName,g_lineNumber,atol(textptr));

    if (tolower(textptr[1])=='x')
        sscanf(textptr+2,"%" SCNx32 "",g_scriptPtr);
    else
        *g_scriptPtr = atol(textptr);

    g_scriptPtr++;

    textptr += l;

    return 0;   // literal value
}

static inline int32_t C_IntPow2(int32_t v)
{
    return ((v!=0) && (v&(v-1))==0);
}

static inline uint32_t C_Pow2IntLogBase2(int32_t v)
{
    static const uint32_t b[] = {0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0,
                                 0xFF00FF00, 0xFFFF0000
                                };
    register uint32_t r = (v & b[0]) != 0;
    int32_t i = 4;

    for (; i > 0; i--)
        r |= ((v & b[i]) != 0) << i;

    return r;
}

static int32_t C_CheckMalformedBranch(ofstype lastScriptOfs)
{
    switch (C_GetKeyword())
    {
    case CON_RIGHTBRACE:
    case CON_ENDEVENT:
    case CON_ENDS:
    case CON_ELSE:
        g_scriptPtr = script + lastScriptOfs;
        cs.ifElseAborted = 1;
        C_CUSTOMWARNING("malformed `%s' branch", keyw[*g_scriptPtr & 0xFFF]);
        return 1;
    }
    return 0;
}

static int32_t C_CheckEmptyBranch(int32_t tw, ofstype lastScriptOfs)
{
    // ifrnd actually does something when the condition is executed
    if ((Bstrncmp(keyw[tw], "if", 2) && tw != CON_ELSE) || tw == CON_IFRND)
    {
        cs.ifElseAborted = 0;
        return 0;
    }

    if ((*(g_scriptPtr) & 0xFFF) != CON_NULLOP || *(g_scriptPtr)>>12 != IFELSE_MAGIC)
        cs.ifElseAborted = 0;

    if (cs.ifElseAborted)
    {
        g_scriptPtr = script + lastScriptOfs;
        C_CUSTOMWARNING("empty `%s' branch", keyw[*g_scriptPtr & 0xFFF]);
        *g_scriptPtr = (CON_NULLOP + (IFELSE_MAGIC<<12));
        return 1;
    }
    return 0;
}

static int32_t C_ParseCommand(void);

static int32_t C_CountCaseStatements()
{
    int32_t lCount;
    const char *temptextptr = textptr;
    int32_t temp_ScriptLineNumber = g_lineNumber;
    ofstype scriptoffset = (unsigned)(g_scriptPtr-script);
    ofstype caseoffset = (unsigned)(cs.caseScriptPtr-script);

    cs.numCases=0;
    cs.caseScriptPtr=NULL;
    //Bsprintf(g_szBuf,"CSS: %.12s",textptr); AddLog(g_szBuf);
    while (C_ParseCommand() == 0)
    {
        //Bsprintf(g_szBuf,"CSSL: %.20s",textptr); AddLog(g_szBuf);
        ;
    }
    // since we processed the endswitch, we need to re-increment cs.checkingSwitch
    cs.checkingSwitch++;

    textptr = temptextptr;
    g_scriptPtr = (instype *)(script+scriptoffset);

    g_lineNumber = temp_ScriptLineNumber;

    lCount = cs.numCases;
    cs.numCases = 0;
    cs.caseScriptPtr = (instype *)(script+caseoffset);
    return lCount;
}

static int32_t C_ParseCommand(void)
{
    int32_t i, j=0, k=0, done, tw;
    const char *temptextptr;
    instype *tempscrptr = NULL;

///    if (quitevent)
///    {
///        initprintf("Aborted.\n");
///        return 1;
///    }

    if (g_numCompilerErrors >= ABORTERRCNT || (*textptr == '\0') || (*(textptr+1) == '\0'))
        return 1;

//    if (g_scriptDebug)
//        C_ReportError(-1);

///    if (cs.checkingSwitch > 0)
///        Bsprintf(g_szBuf,"PC(): '%.25s'",textptr); AddLog(g_szBuf);

    tw = C_GetNextKeyword();
    //    Bsprintf(tempbuf,"%s",keyw[tw]); AddLog(tempbuf);

    if (C_SkipComments())
        return 1;

    switch (tw)
    {
    default:
    case -1:
        return 0; //End

// *** basic commands
    case CON_NULLOP:
        if (C_GetKeyword() != CON_ELSE)
        {
            C_CUSTOMWARNING("`nullop' found without `else'");
            g_scriptPtr--;
            cs.ifElseAborted = 1;
        }
        return 0;

    case CON_DEFINE:
    {
        C_GetNextLabelName();

        if (hash_find(&h_keywords, tlabel) >= 0)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_ISAKEYWORD);
            return 0;
        }

        // Check to see it's already defined
        i = hash_find(&h_gamevars, tlabel);
        if (i>=0)
        {
            g_numCompilerWarnings++;
            C_ReportError(WARNING_NAMEMATCHESVAR);
        }

        if (hash_find(&h_states, tlabel) >= 0)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_LABELINUSE);
            return 1;
        }

        C_GetNextValue(LABEL_DEFINE);

        i = hash_find(&h_labels, tlabel);
        if (i == -1)
        {
            // printf("Defining Definition '%s' to be '%d'\n",label+(g_numLabels*MAXLABELLEN),*(g_scriptPtr-1));
//            Bmemcpy(label+(g_numLabels*MAXLABELLEN), tlabel, MAXLABELLEN);
            C_CopyLabel();
            hash_add(&h_labels, label+(g_numLabels*MAXLABELLEN), g_numLabels);
            labeltype[g_numLabels] = LABEL_DEFINE;
            labelval[g_numLabels++] = *(g_scriptPtr-1);
        }
//        else if (i>=g_numDefaultLabels)
//        {
//            if (labeltype[i] == LABEL_DEFINE)
//                labelval[i] = *(g_scriptPtr-1);
//        }

        g_scriptPtr -= 2;
        return 0;
    }

    case CON_INCLUDE:
        g_scriptPtr--;
        while (!isaltok(*textptr))
        {
            g_lineNumber += (*textptr == 0x0a);

            textptr++;
            if (!*textptr)
                break;
        }

        j = 0;
        while (isaltok(*textptr))
        {
            tempbuf[j] = *(textptr++);
            j++;
        }
        tempbuf[j] = '\0';

        {
            int32_t temp_ScriptLineNumber;
            int32_t  temp_ifelse_check;
            const char *origtptr;
            char *mptr;
            char parentScriptFileName[255];
            int32_t fp;

            fp = kopen4load(tempbuf, 0 /*g_loadFromGroupOnly*/);
            if (fp < 0)
            {
                g_numCompilerErrors++;
                initprintf("%s:%d: error: could not find file `%s'.\n",g_szScriptFileName,g_lineNumber,tempbuf);
                return 1;
            }

            j = kfilelength(fp);

            mptr = (char *)Bmalloc(j+1);
            if (!mptr)
            {
                kclose(fp);
                g_numCompilerErrors++;
                initprintf("%s:%d: error: could not allocate %d bytes to include `%s'.\n",
                           g_lineNumber,g_szScriptFileName,j,tempbuf);
                return 1;
            }

            initprintf("  Including: %s (%d bytes)\n", tempbuf, j);
            kread(fp, mptr, j);
            kclose(fp);
            mptr[j] = 0;

            if (*textptr == '"') // skip past the closing quote if it's there so we don't screw up the next line
                textptr++;
            origtptr = textptr;

            Bstrcpy(parentScriptFileName, g_szScriptFileName);
            Bstrcpy(g_szScriptFileName, tempbuf);
            temp_ScriptLineNumber = g_lineNumber;
            g_lineNumber = 1;
            temp_ifelse_check = cs.checkingIfElse;
            cs.checkingIfElse = 0;

            textptr = mptr;
            do done = C_ParseCommand();
            while (!done);

            Bstrcpy(g_szScriptFileName, parentScriptFileName);
            g_totalLines += g_lineNumber;
            g_lineNumber = temp_ScriptLineNumber;
            cs.checkingIfElse = temp_ifelse_check;

            textptr = origtptr;

            Bfree(mptr);
        }
        return 0;

    case CON_DEFSTATE:
        g_scriptPtr--;
        if (cs.parsingEventOfs < 0  && cs.currentStateIdx < 0)
        {
            C_GetNextLabelName();

            if (hash_find(&h_keywords, tlabel)>=0)
            {
                g_numCompilerErrors++;
                C_ReportError(ERROR_ISAKEYWORD);
                return 1;
            }

            if (hash_find(&h_gamevars, tlabel)>=0)
            {
                g_numCompilerWarnings++;
                C_ReportError(WARNING_NAMEMATCHESVAR);
            }

            j = hash_find(&h_labels, tlabel);
            if (j>=0)
            {
                g_numCompilerErrors++;
                C_ReportError(ERROR_LABELINUSE);
                return 1;
            }

            j = hash_find(&h_states, tlabel);
            if (j>=0)  // only redefining
            {
                cs.currentStateIdx = j;
                cs.currentStateOfs = (g_scriptPtr-script);

                Bsprintf(g_szCurrentBlockName, "%s", statesinfo[j].name);
            }
            else  // new state definition
            {
                cs.currentStateIdx = j = g_stateCount;
                cs.currentStateOfs = (g_scriptPtr-script);

                if (g_stateCount >= statesinfo_allocsize)
                {
                    statesinfo_allocsize *= 2;
                    statesinfo = Brealloc(statesinfo, statesinfo_allocsize * sizeof(statesinfo[0]));
                    if (!statesinfo)
                    {
                        initprintf("C_ParseCommand(): ERROR: out of memory!\n");
                        g_numCompilerErrors++;
                        return 1;
                    }
                }

                Bmemcpy(statesinfo[j].name, tlabel, MAXLABELLEN);
                Bsprintf(g_szCurrentBlockName, "%s", tlabel);
                hash_add(&h_states, tlabel, j);
            }

            return 0;
        }

        g_numCompilerErrors++;
        C_ReportError(ERROR_FOUNDWITHIN);
        return 1;

    case CON_ENDS:
        if (cs.currentStateIdx < 0)
        {
            C_CUSTOMERROR("found `ends' without open `state'.");
            return 1; //+
        }
        else
        {
            if (cs.numBraces > 0)
            {
                C_ReportError(ERROR_OPENBRACKET);
                g_numCompilerErrors++;
            }
            else if (cs.numBraces < 0)
            {
                C_ReportError(ERROR_CLOSEBRACKET);
                g_numCompilerErrors++;
            }

            if (cs.checkingSwitch > 0)
            {
                C_ReportError(ERROR_NOENDSWITCH);
                g_numCompilerErrors++;

                cs.checkingSwitch = 0; // can't be checking anymore...
            }

            if (g_numCompilerErrors)
            {
                g_scriptPtr = script+cs.currentStateOfs;
                cs.currentStateOfs = -1;
                cs.currentStateIdx = -1;
                Bsprintf(g_szCurrentBlockName,"(none)");
                return 0;
            }

            j = cs.currentStateIdx;

            if (cs.currentStateIdx == g_stateCount)  // we were defining a new state
            {
                statesinfo[j].ofs = cs.currentStateOfs;
                statesinfo[j].codesize = (g_scriptPtr-script) - cs.currentStateOfs;

                g_stateCount++;

                initprintf("  Defined state `%s' (index %d).\n", g_szCurrentBlockName, j);
//                initprintf("    o:%d s:%d\n", statesinfo[j].ofs, statesinfo[j].codesize);
            }
            else  // we were redefining a state
            {
                int32_t oofs = statesinfo[j].ofs;
                int32_t nofs = cs.currentStateOfs;
                int32_t osize = statesinfo[j].codesize;
                int32_t nsize = (g_scriptPtr-script) - nofs;

                if (nsize == osize)
                {
                    int ii, equal=2, linedif = 0, ow, nw;

                    for (ii=0; ii<nsize; ii++)
                    {
                        ow = *(script+oofs+ii);
                        nw = *(script+nofs+ii);
                        if (ow != nw)
                        {
                            int32_t ld = (nw>>12) - (ow>>12);
                            if (equal==2)
                            {
                                equal = 1;
                                linedif = ld;
                            }

                            if (linedif != ld || ((nw&0xFFF) != (ow&0xFFF)))
                            {
                                equal = 0;
                                break;
                            }
                        }
                    }

                    if (equal!=2)
                        Bmemcpy(script+oofs, script+nofs, nsize*sizeof(instype));
                    if (equal==0)
                        initprintf("  Redefined state `%s' (index %d).\n", g_szCurrentBlockName, j);
//                        initprintf("    oo:%d os:%d, no:%d ns:%d\n", oofs, osize, nofs, nsize);
                }
                else
                {
                    int32_t ii;
                    uint32_t movedcodesize = g_scriptPtr - (script+oofs+osize);

                    Bmemmove(script+oofs, script+oofs+osize, movedcodesize*sizeof(instype));

                    for (ii=0; ii<g_stateCount; ii++)
                    {
                        if (statesinfo[ii].ofs > oofs)
                            statesinfo[ii].ofs -= osize;
                    }
                    for (ii=0; ii<MAXEVENTS; ii++)
                        if (/*ii != j &&*/ aEventOffsets[ii] > oofs)
                            aEventOffsets[ii] -= osize;

                    statesinfo[j].ofs = nofs-osize;
                    statesinfo[j].codesize = nsize;

                    initprintf("  Redefined state `%s' (index %d).\n", g_szCurrentBlockName, j);
//                    initprintf("    oo:%d os:%d, no:%d ns:%d\n", oofs, osize, nofs, nsize);
                }
                g_scriptPtr -= osize;
            }

            g_didDefineSomething = 1;

            cs.currentStateOfs = -1;
            cs.currentStateIdx = -1;

            Bsprintf(g_szCurrentBlockName,"(none)");
        }
        return 0;

    case CON_SORT:
        C_GetNextLabelName();
        i = GetGamearrayID(tlabel);
        if (i >= 0)
        {
            *g_scriptPtr++ = i;
            if (aGameArrays[i].dwFlags & GAMEARRAY_READONLY)
            {
                C_ReportError(ERROR_ARRAYREADONLY);
                g_numCompilerErrors++;
            }
        }
        else
        {
            C_ReportError(ERROR_NOTAGAMEARRAY);
            g_numCompilerErrors++;
        }
        C_SkipComments();
        C_GetNextVar();  // element count to sort

        if (C_GetKeyword() >= 0)
        {
            *g_scriptPtr++ = -1;
            return 0;
        }
        // fall-through
    case CON_STATE:
        C_GetNextLabelName();

        if (hash_find(&h_keywords, tlabel)>=0)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_ISAKEYWORD);
            return 1;
        }

        if (hash_find(&h_gamevars, tlabel)>=0)
        {
            g_numCompilerWarnings++;
            C_ReportError(WARNING_NAMEMATCHESVAR);
        }

        j = hash_find(&h_states, tlabel);
        if (j>=0)
        {
//            if (!(g_numCompilerErrors || g_numCompilerWarnings) && g_scriptDebug > 1)
//                initprintf("%s:%d: debug: accepted state label `%s'.\n",g_szScriptFileName,g_lineNumber,label+(k*MAXLABELLEN));
            *g_scriptPtr++ = j;
            return 0;
        }

        C_CUSTOMERROR("state `%s' not found.", tlabel);
        g_scriptPtr++;
        return 0;

    case CON_ONEVENT:
        if (cs.currentStateIdx >= 0 || cs.parsingEventOfs >= 0)
        {
            C_ReportError(ERROR_FOUNDWITHIN);
            g_numCompilerErrors++;
        }

        g_scriptPtr--;
        cs.numBraces = 0;

        C_SkipComments();
        j = 0;
        while (isaltok(*(textptr+j)))
        {
            g_szCurrentBlockName[j] = textptr[j];
            j++;
        }
        g_szCurrentBlockName[j] = 0;

        cs.labelsOnly = 1;
        C_GetNextValue(LABEL_EVENT);
        cs.labelsOnly = 0;

        g_scriptPtr--;
        j = *g_scriptPtr;  // event number

        cs.currentEvent = j;
        cs.parsingEventOfs = g_scriptPtr-script;
        //Bsprintf(g_szBuf,"Adding Event for %d at %lX",j, g_parsingEventPtr); AddLog(g_szBuf);
        if (j<0 || j >= MAXEVENTS)
        {
            initprintf("%s:%d: error: invalid event ID.\n",g_szScriptFileName,g_lineNumber);
            g_numCompilerErrors++;
            return 0;
        }

        cs.checkingIfElse = 0;
        return 0;

    case CON_ENDEVENT:
        if (cs.parsingEventOfs < 0)
        {
            C_CUSTOMERROR("found `endevent' without open `onevent'.");
            return 1;
        }
        if (cs.numBraces > 0)
        {
            C_ReportError(ERROR_OPENBRACKET);
            g_numCompilerErrors++;
        }
        if (cs.numBraces < 0)
        {
            C_ReportError(ERROR_CLOSEBRACKET);
            g_numCompilerErrors++;
        }
        if (g_numCompilerErrors)
        {
            g_scriptPtr = script+cs.parsingEventOfs;
            cs.parsingEventOfs = -1;
            cs.currentEvent = -1;
            Bsprintf(g_szCurrentBlockName, "(none)");
            return 0;
        }

        j = cs.currentEvent;
        if (aEventOffsets[j] >= 0)  // if event was previously declared, overwrite it
        {
            int32_t oofs = aEventOffsets[j], nofs = cs.parsingEventOfs;
            int32_t osize = aEventSizes[j], nsize = (g_scriptPtr-script) - nofs;

            if (osize == nsize)
            {
                int ii, equal=2, linedif=0, nw, ow;

                for (ii=0; ii<nsize; ii++)
                {
                    ow = *(script+oofs+ii);
                    nw = *(script+nofs+ii);
                    if (ow != nw)
                    {
                        int32_t ld = (nw>>12) - (ow>>12);
                        if (equal==2)
                        {
                            equal = 1;
                            linedif = ld;
                        }

                        if (linedif != ld || ((nw&0xFFF) != (ow&0xFFF)))
                        {
                            equal = 0;
                            break;
                        }
                    }
                }

                if (equal!=2)
                    Bmemcpy(script+oofs, script+nofs, nsize*sizeof(instype));
                if (equal==0)
                    initprintf("  Redefined event `%s' (index %d).\n", g_szCurrentBlockName, j);
//                        initprintf("    oo:%d os:%d, no:%d ns:%d\n", oofs, osize, nofs, nsize);
            }
            else
            {
                int32_t ii;
                uint32_t movedcodesize = g_scriptPtr - (script+oofs + osize);

                Bmemmove(script+oofs, script+oofs + osize, movedcodesize*sizeof(instype));

                for (ii=0; ii<g_stateCount; ii++)
                {
                    if (statesinfo[ii].ofs > oofs)
                        statesinfo[ii].ofs -= osize;
                }
                for (ii=0; ii<MAXEVENTS; ii++)
                    if (/*ii != j &&*/ aEventOffsets[ii] > oofs)
                        aEventOffsets[ii] -= osize;

                aEventOffsets[j] = nofs - osize;
                aEventSizes[j] = nsize;

                initprintf("  Redefined event `%s' (index %d).\n", g_szCurrentBlockName, j);
//                initprintf("    oo:%d os:%d, no:%d ns:%d\n", oofs, osize, nofs, nsize);
            }
            g_scriptPtr -= osize;
        }
        else  // event defined for the first time
        {
            aEventOffsets[j] = cs.parsingEventOfs;
            aEventSizes[j] = (g_scriptPtr-script) - cs.parsingEventOfs;

            initprintf("  Defined event `%s' (index %d).\n", g_szCurrentBlockName, j);
//            initprintf("    o:%d s:%d\n", aEventOffsets[j], aEventSizes[j]);
        }

        g_didDefineSomething = 1;

        cs.parsingEventOfs = -1;
        cs.currentEvent = -1;
        Bsprintf(g_szCurrentBlockName,"(none)");

        return 0;

// *** control flow
    case CON_ELSE:
        if (cs.checkingIfElse)
        {
            ofstype offset;
            ofstype lastScriptOfs = (g_scriptPtr-script) - 1;
            instype *tscrptr;

            cs.ifElseAborted = 0;
            cs.checkingIfElse--;

            if (C_CheckMalformedBranch(lastScriptOfs))
                return 0;

            offset = (unsigned)(g_scriptPtr-script);

            g_scriptPtr++; //Leave a spot for the fail location
            C_ParseCommand();

            if (C_CheckEmptyBranch(tw, lastScriptOfs))
                return 0;

            tscrptr = (instype *)script+offset;
            *tscrptr = (ofstype)(g_scriptPtr-script)-offset;   // relative offset
        }
        else
        {
            instype *tscrptr;

            g_scriptPtr--;
            tscrptr = g_scriptPtr;

            C_CUSTOMWARNING("found `else' with no `if'.");

            if (C_GetKeyword() == CON_LEFTBRACE)
            {
                C_GetNextKeyword();
                cs.numBraces++;

                do
                    done = C_ParseCommand();
                while (done == 0);
            }
            else C_ParseCommand();

            g_scriptPtr = tscrptr;
        }
        return 0;

    case CON_RETURN:
    case CON_BREAK:
        if (cs.checkingSwitch)
        {
            //Bsprintf(g_szBuf,"  * (L%d) case Break statement.\n",g_lineNumber); AddLog(g_szBuf);
            return 1;
        }
        return 0;

    case CON_SWITCH:
    {
        ofstype tempoffset;

        //AddLog("Got Switch statement");
        //        if (cs.checkingSwitch) Bsprintf(g_szBuf,"ERROR::%s %d: cs.checkingSwitch=",__FILE__,__LINE__, cs.checkingSwitch); AddLog(g_szBuf);
        cs.checkingSwitch++;  // allow nesting (if other things work)

        C_GetNextVar();  // Get The ID of the DEF

        tempoffset = (unsigned)(g_scriptPtr-script);

        *g_scriptPtr++ = 0; // leave spot for end location (for after processing)
        *g_scriptPtr++ = 0; // count of case statements

        cs.caseScriptPtr = g_scriptPtr;        // the first case's pointer.
        *g_scriptPtr++ = -1; // leave spot for 'default' offset to cases' code (-1 if none)

        temptextptr = textptr;
        // probably does not allow nesting...
        //AddLog("Counting Case Statements...");
        j = C_CountCaseStatements();
        //        initprintf("Done Counting Case Statements for switch %d: found %d.\n", cs.checkingSwitch,j);
        g_scriptPtr += j*2;
        cs.caseCodePtr = g_scriptPtr;
        C_SkipComments();
        g_scriptPtr -= j*2; // allocate buffer for the table

        tempscrptr = (instype *)(script+tempoffset);
        //        if (cs.checkingSwitch>1) Bsprintf(g_szBuf,"ERROR::%s %d: cs.checkingSwitch=",__FILE__,__LINE__, cs.checkingSwitch);  AddLog(g_szBuf);
        if (j<0)
            return 1;

        if (tempscrptr)
            tempscrptr[1] = j;  // save count of cases
//        else
//            Bsprintf(g_szBuf,"ERROR::%s %d",__FILE__,__LINE__); AddLog(g_szBuf);

        while (j--)
        {
            // leave room for statements
            *g_scriptPtr++ = 0; // value check
            *g_scriptPtr++ = -1; // code offset
            C_SkipComments();
        }
        //Bsprintf(g_szBuf,"SWITCH1: '%.22s'",textptr); AddLog(g_szBuf);
        cs.numCases = 0;
        while (C_ParseCommand() == 0)
        {
            //Bsprintf(g_szBuf,"SWITCH2: '%.22s'",textptr); AddLog(g_szBuf);
        }

        tempscrptr = (instype *)(script+tempoffset);
        //Bsprintf(g_szBuf,"SWITCHXX: '%.22s'",textptr); AddLog(g_szBuf);
        // done processing switch.  clean up.
        //        if (cs.checkingSwitch < 1) Bsprintf(g_szBuf,"ERROR::%s %d: cs.checkingSwitch=%d",__FILE__,__LINE__, cs.checkingSwitch); AddLog(g_szBuf);
        if (tempscrptr)
        {
            int32_t t,n;   // !!!
            for (i=3; i<3+tempscrptr[1]*2-2; i+=2) // sort them
            {
                t = tempscrptr[i]; n=i;
                for (j=i+2; j<3+tempscrptr[1]*2; j+=2)
                    if (tempscrptr[j] < t)
                        t = tempscrptr[j], n=j;
                if (n != i)
                {
                    t = tempscrptr[i];
                    tempscrptr[i] = tempscrptr[n];
                    tempscrptr[n] = t;

                    t = tempscrptr[i+1];
                    tempscrptr[i+1] = tempscrptr[n+1];
                    tempscrptr[n+1] = t;
                }
            }
//            for (j=3;j<3+tempscrptr[1]*2;j+=2)initprintf("%5d %8x\n",tempscrptr[j],tempscrptr[j+1]);
            tempscrptr[0] = (ofstype)(g_scriptPtr-cs.caseCodePtr);    // save 'end' location as offset from code-place
        }
        //        else Bsprintf(g_szBuf,"ERROR::%s %d",__FILE__,__LINE__); AddLog(g_szBuf);
        cs.numCases = 0;
        cs.caseScriptPtr = NULL;
        cs.caseCodePtr = NULL;
        // decremented in endswitch.  Don't decrement here...
        //                    cs.checkingSwitch--; // allow nesting (maybe if other things work)
        tempscrptr = NULL;
        //        if (cs.checkingSwitch) Bsprintf(g_szBuf,"ERROR::%s %d: cs.checkingSwitch=%d",__FILE__,__LINE__, cs.checkingSwitch); AddLog(g_szBuf);
        //AddLog("End of Switch statement");
    }
    break;

    case CON_CASE:
    {
        ofstype tempoffset = 0;
        //AddLog("Found Case");
repeatcase:
        g_scriptPtr--; // don't save in code
        if (cs.checkingSwitch < 1)
        {
            C_CUSTOMERROR("found `case' statement when not in switch");
            return 1;
        }

        cs.numCases++;
        //Bsprintf(g_szBuf,"case1: %.12s",textptr); AddLog(g_szBuf);
        C_GetNextValue(LABEL_DEFINE);
        if (*textptr == ':')
            textptr++;
        //Bsprintf(g_szBuf,"case2: %.12s",textptr); AddLog(g_szBuf);
        j = *(--g_scriptPtr);      // get value
        //Bsprintf(g_szBuf,"case: Value of case %d is %d",(int32_t)cs.numCases,(int32_t)j); AddLog(g_szBuf);
        if (cs.caseScriptPtr)
        {
            for (i=(cs.numCases/2)-1; i>=0; i--)
                if (cs.caseScriptPtr[i*2+1] == j)
                {
                    g_numCompilerWarnings++;
                    C_ReportError(WARNING_DUPLICATECASE);
                    break;
                }
            //AddLog("Adding value to script");
            cs.caseScriptPtr[cs.numCases++] = j;   // save value
            cs.caseScriptPtr[cs.numCases] = (ofstype)(g_scriptPtr - cs.caseCodePtr);  // offset from beginning of cases' code
        }
        //Bsprintf(g_szBuf,"case3: %.12s",textptr); AddLog(g_szBuf);
        j = C_GetKeyword();
        if (j == CON_CASE)
        {
            //AddLog("Found Repeat Case");
            C_GetNextKeyword();    // eat 'case'
            goto repeatcase;
        }
        //Bsprintf(g_szBuf,"case4: '%.12s'",textptr); AddLog(g_szBuf);
        tempoffset = (unsigned)(tempscrptr-script);
        while (C_ParseCommand() == 0)
        {
            //Bsprintf(g_szBuf,"case5 '%.25s'",textptr); AddLog(g_szBuf);
            j = C_GetKeyword();
            if (j == CON_CASE)
            {
                //AddLog("Found Repeat Case");
                C_GetNextKeyword();    // eat 'case'
                tempscrptr = (instype *)(script+tempoffset);
                goto repeatcase;
            }
        }
        tempscrptr = (instype *)(script+tempoffset);
        //AddLog("End Case");
        return 0;
        //      break;
    }

    case CON_DEFAULT:
        g_scriptPtr--;    // don't save
        if (cs.checkingSwitch < 1)
        {
            C_CUSTOMERROR("found `default' statement when not in switch");
            return 1;
        }
        if (cs.caseScriptPtr && cs.caseScriptPtr[0]!=0)
        {
            C_CUSTOMERROR("multiple `default' statements found in switch");
        }

        if (cs.caseScriptPtr)
            cs.caseScriptPtr[0] = (ofstype)(g_scriptPtr-cs.caseCodePtr);   // save offset from cases' code
        //Bsprintf(g_szBuf,"default: '%.22s'",textptr); AddLog(g_szBuf);

        while (C_ParseCommand() == 0)
        {
            //Bsprintf(g_szBuf,"defaultParse: '%.22s'",textptr); AddLog(g_szBuf);
            ;
        }

        break;

    case CON_ENDSWITCH:
        //AddLog("End Switch");
        cs.checkingSwitch--;

        if (cs.checkingSwitch < 0)
            C_CUSTOMERROR("found `endswitch' without matching `switch'");

        return 1;      // end of block
//        break;

    case CON_GETCURRADDRESS:
        C_GetNextVarType(GAMEVAR_READONLY);
        return 0;

    case CON_JUMP:
        C_GetNextVar();
        return 0;

    case CON_LEFTBRACE:
//        if (!(cs.currentStateIdx >= 0 || cs.parsingEventOfs >= 0))
//        {
//            g_numCompilerErrors++;
//            C_ReportError(ERROR_SYNTAXERROR);
//        }

        cs.numBraces++;

        do done = C_ParseCommand();
        while (!done);

        return 0;

    case CON_RIGHTBRACE:
        cs.numBraces--;

        // rewrite "{ }" into "nullop"
        if (*(g_scriptPtr-2) == CON_LEFTBRACE + (IFELSE_MAGIC<<12))
        {
//            initprintf("%s:%d: rewriting empty braces '{ }' as 'nullop' from right\n",g_szScriptFileName,g_lineNumber);
            *(g_scriptPtr-2) = CON_NULLOP + (IFELSE_MAGIC<<12);
            g_scriptPtr -= 2;

            if (C_GetKeyword() != CON_ELSE && (*(g_scriptPtr-2)&0xFFF) != CON_ELSE)
                cs.ifElseAborted = 1;
            else
                cs.ifElseAborted = 0;

            j = C_GetKeyword();

            if (cs.checkingIfElse && j != CON_ELSE)
                cs.checkingIfElse--;

            return 1;
        }

        if (cs.numBraces < 0)
        {
            if (cs.checkingSwitch)
                C_ReportError(ERROR_NOENDSWITCH);

            C_CUSTOMERROR("found more `}' than `{'.");
        }
        if (cs.checkingIfElse && j != CON_ELSE)
            cs.checkingIfElse--;

        return 1;

// *** more basic commands
    case CON_SETSECTOR:
    case CON_GETSECTOR:
    case CON_SETWALL:
    case CON_GETWALL:
    case CON_SETSPRITE:
    case CON_GETSPRITE:
    case CON_SETTSPR:
    case CON_GETTSPR:
    {
        int32_t lLabelID;

        // syntax getsector[<var>].x <VAR>
        // gets the value of sector[<var>].xxx into <VAR>

        if ((tw==CON_GETTSPR || tw==CON_SETTSPR) && cs.currentEvent != EVENT_ANALYZESPRITES)
        {
            C_ReportError(WARNING_OUTSIDEDRAWSPRITE);
            g_numCompilerWarnings++;
        }

        // now get name of .xxx
        while (*textptr != '[')
            textptr++;

        if (*textptr == '[')
            textptr++;

        // get the ID of the DEF
        cs.labelsOnly = 1;
        C_GetNextVar();
        cs.labelsOnly = 0;

        // now get name of .xxx
        while (*textptr != '.')
        {
            if (!*textptr || *textptr == 0xa)
                break;

            textptr++;
        }

        if (*textptr!='.')
        {
            C_CUSTOMERROR("error: syntax error. Expected `.<label>' after `%s[...]'", keyw[tw]);
            return 0;
        }
        textptr++;

        /// now pointing at 'xxx'
        C_GetNextLabelName();
        //printf("found xxx label of '%s'\n",   label+(g_numLabels*MAXLABELLEN));

        if (tw==CON_GETSECTOR || tw==CON_SETSECTOR)
            lLabelID = C_GetLabelNameID(SectorLabels, &h_sector, Bstrtolower(tlabel));
        else if (tw==CON_GETWALL || tw==CON_SETWALL)
            lLabelID = C_GetLabelNameID(WallLabels, &h_wall, Bstrtolower(tlabel));
        else // if (tw==CON_GETSPRITE || tw==CON_SETSPRITE || tw==CON_GETTSPR || tw==CON_SETTSPR)
            lLabelID = C_GetLabelNameID(SpriteLabels, &h_sprite, Bstrtolower(tlabel));

        if (lLabelID == -1)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_SYMBOLNOTRECOGNIZED);
            return 0;
        }

        *g_scriptPtr++ = lLabelID;

        // now at target VAR...
        // get the ID of the DEF
        if (tw==CON_GETSECTOR || tw==CON_GETWALL || tw==CON_GETSPRITE || tw==CON_GETTSPR)
            C_GetNextVarType(GAMEVAR_READONLY);
        else
            C_GetNextVar();
        break;
    }

    case CON_GAMEVAR:
        // syntax: gamevar <var1> <initial value> <flags>
        // defines var1 and sets initial value.
        // flags are used to define usage
        // (see top of this files for flags)
        //printf("Got gamedef. Getting Label. '%.20s'\n",textptr);

        if (isdigit(*textptr) || (*textptr == '-'))
        {
            C_CUSTOMERROR("error: syntax error. Expected identifier after `gamevar'");
            C_GetNextLabelName();
            C_GetNextValue(LABEL_DEFINE);
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr -= 3; // we complete the process anyways just to skip past the fucked up section
            return 0;
        }

        C_GetNextLabelName();
        //printf("Got Label '%.20s'\n",textptr);
        // Check to see it's already defined

        if (hash_find(&h_keywords, tlabel)>=0)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_ISAKEYWORD);
            return 0;
        }

        C_GetNextValue(LABEL_DEFINE); // get initial value
        C_GetNextValue(LABEL_DEFINE); // get flags
        //Bsprintf(g_szBuf,"Adding GameVar='%s', val=%l, flags=%lX",label+(g_numLabels*MAXLABELLEN), *(g_scriptPtr-2), *(g_scriptPtr-1)); AddLog(g_szBuf);
///        if ((*(g_scriptPtr-1)&GAMEVAR_USER_MASK)==3)
///        {
///            g_numCompilerWarnings++;
///            *(g_scriptPtr-1) ^= GAMEVAR_PERBLOCK;
///            C_ReportError(WARNING_BADGAMEVAR);
///        }

        Gv_NewVar(tlabel, *(g_scriptPtr-2),
                  /* can't define default or secret */
                  *(g_scriptPtr-1) & ~(GAMEVAR_DEFAULT|GAMEVAR_SECRET));

        //AddLog("Added gamevar");
        g_scriptPtr -= 3; // no need to save in script...
        return 0;

// *** arrays
    case CON_GAMEARRAY:
        if (isdigit(*textptr) || (*textptr == '-'))
        {
            C_CUSTOMERROR("error: syntax error. Expected identifier after `gamearray'");
            C_GetNextLabelName();
            C_GetNextValue(LABEL_DEFINE);
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr -= 2; // we complete the process anyways just to skip past the fucked up section
            return 0;
        }

        C_GetNextLabelName();
        //printf("Got Label '%.20s'\n",textptr);
        // Check to see it's already defined
        if (hash_find(&h_keywords, tlabel) >= 0)
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_ISAKEYWORD);
            return 0;
        }

        if (hash_find(&h_gamevars, tlabel) >= 0)
        {
            g_numCompilerWarnings++;
            C_ReportError(WARNING_NAMEMATCHESVAR);
        }

        C_GetNextValue(LABEL_DEFINE);
        Gv_NewArray(tlabel, 0, *(g_scriptPtr-1), GAMEARRAY_NORMAL);

        g_scriptPtr -= 2; // no need to save in script...
        return 0;

    case CON_COPY:
        C_GetNextLabelName();

        i = GetGamearrayID(tlabel);
        if (i >= 0)
            *g_scriptPtr++ = i;
        else
        {
            C_ReportError(ERROR_NOTAGAMEARRAY);
            g_numCompilerErrors++;
        }

        C_SkipComments();

        if (*textptr != '[')
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_GAMEARRAYBNO);
            return 1;
        }
        textptr++;

        C_GetNextVar();
        C_SkipComments();

        if (*textptr != ']')
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_GAMEARRAYBNC);
            return 1;
        }
        textptr++;
        // fall-through
    case CON_SETARRAY:
        C_GetNextLabelName();

        i = GetGamearrayID(tlabel);
        if (i >= 0)
        {
            *g_scriptPtr++ = i;

            if (aGameArrays[i].dwFlags & GAMEARRAY_READONLY)
            {
                C_ReportError(ERROR_ARRAYREADONLY);
                g_numCompilerErrors++;
            }
        }
        else
        {
            C_ReportError(ERROR_NOTAGAMEARRAY);
            g_numCompilerErrors++;
        }

        C_SkipComments();

        if (*textptr != '[')
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_GAMEARRAYBNO);
            return 1;
        }
        textptr++;

        C_GetNextVar();
        C_SkipComments();

        if (*textptr != ']')
        {
            g_numCompilerErrors++;
            C_ReportError(ERROR_GAMEARRAYBNC);
            return 1;
        }
        textptr++;
        C_GetNextVar();
        return 0;

    case CON_GETARRAYSIZE:
    case CON_RESIZEARRAY:
        C_GetNextLabelName();
        i = GetGamearrayID(tlabel);
        if (i >= 0)
        {
            *g_scriptPtr++ = i;
            if (tw==CON_RESIZEARRAY && (aGameArrays[i].dwFlags&(GAMEARRAY_TYPEMASK)))
            {
                C_CUSTOMERROR("can't resize system array `%s'.", tlabel);
            }
        }
        else
        {
            C_ReportError(ERROR_NOTAGAMEARRAY);
            g_numCompilerErrors++;
        }
        C_SkipComments();
        C_GetNextVar();
        return 0;

#if 0
    case CON_WRITEARRAYTOFILE:
    case CON_READARRAYFROMFILE:
        C_GetNextLabelName();
        i = GetGamearrayID(tlabel);
        if (i > (-1))
        {
            *g_scriptPtr++ = i;
        }
        else
        {
            C_ReportError(ERROR_NOTAGAMEARRAY);
            g_numCompilerErrors++;
        }
        C_GetNextValue(LABEL_DEFINE);
        return 0;
#endif

// *** var ops
    case CON_RANDVAR:
    case CON_DISPLAYRANDVAR:
    case CON_SETVAR:
    case CON_ADDVAR:
    case CON_SUBVAR:
    case CON_MULVAR:
    case CON_DIVVAR:
    case CON_MODVAR:
    case CON_ANDVAR:
    case CON_ORVAR:
    case CON_XORVAR:
    case CON_SHIFTVARL:
    case CON_SHIFTVARR:
    {
        instype *inst = g_scriptPtr-1;
        const char *tptr = textptr;
        // syntax: [rand|add|set]var    <var1> <const1>
        // sets var1 to const1
        // adds const1 to var1 (const1 can be negative...)
        //printf("Found [add|set]var at line= %d\n",g_lineNumber);

        // get the ID of the DEF
        C_GetNextVarType(GAMEVAR_READONLY);

        C_GetNextValue(LABEL_DEFINE); // the number to check against...

        if (tw == CON_MULVAR && *(g_scriptPtr-1) == -1)
        {
            *inst = CON_INV;
            g_scriptPtr--;
            return 0;
        }

        if (tw == CON_DIVVAR || (tw == CON_MULVAR && *(g_scriptPtr-1) > 0))
        {
            int32_t i = *(g_scriptPtr-1);
            j = klabs(i);

            if (C_IntPow2(j))
            {
                *inst = ((tw == CON_DIVVAR) ? CON_SHIFTVARR : CON_SHIFTVARL);
                *(g_scriptPtr-1) = C_Pow2IntLogBase2(j);
//                    initprintf("%s:%d: replacing multiply/divide with shift\n",g_szScriptFileName,g_lineNumber);

                if (i == j)
                    return 0;

                *g_scriptPtr++ = CON_INV + (g_lineNumber<<12);
                textptr = tptr;
                C_GetNextVarType(GAMEVAR_READONLY);
                C_GetNextValue(LABEL_DEFINE);
                g_scriptPtr--;
//                    initprintf("%s:%d: adding inversion\n",g_szScriptFileName,g_lineNumber);
            }
        }
    }
    return 0;

// *** varvar ops
    case CON_RANDVARVAR:
    case CON_DISPLAYRANDVARVAR:
    case CON_SETVARVAR:
    case CON_ADDVARVAR:
    case CON_SUBVARVAR:
    case CON_MULVARVAR:
    case CON_DIVVARVAR:
    case CON_MODVARVAR:
    case CON_ANDVARVAR:
    case CON_ORVARVAR:
    case CON_XORVARVAR:
    {
        instype *inst = (g_scriptPtr-1);
        const char *otextptr;

        C_GetNextVarType(GAMEVAR_READONLY);

        otextptr = textptr;

        g_wasConstant = 0;
        C_GetNextVar();

        if (!g_numCompilerErrors && g_wasConstant)
        {
            textptr = otextptr;
            g_scriptPtr--;
            *inst -= (CON_SETVARVAR - CON_SETVAR);
            C_GetNextValue(LABEL_DEFINE);
        }
        return 0;
    }

    case CON_SIN:
    case CON_COS:
        C_GetNextVarType(GAMEVAR_READONLY);
        C_GetNextVar();
        return 0;

// *** random
    case CON_DISPLAYRAND:
        // syntax: displayrand <var>
        // gets rand (not game rand) into <var>

        C_GetNextVarType(GAMEVAR_READONLY);
        break;

// *** float access: convert float to int and back
    case CON_ITOF:
    case CON_FTOI:
        // syntax: itof <<var>> SCALE
        //         ftoi <<var>> SCALE
        C_GetNextVarType(GAMEVAR_READONLY);
        C_GetNextValue(LABEL_DEFINE);

        if (*(g_scriptPtr-1)<=0)
            C_CUSTOMERROR("scale value in integer/float conversion must be greater zero.");

        return 0;

// *** other math
    case CON_CLAMP:
        C_GetNextVarType(GAMEVAR_READONLY);
        C_GetManyVars(2);
        return 0;

    case CON_INV:
        C_GetNextVarType(GAMEVAR_READONLY);
        return 0;

    case CON_SQRT:
    {
        // syntax sqrt <invar> <outvar>
        // gets the sqrt of invar into outvar

        C_GetNextVar();
        // target var
        C_GetNextVarType(GAMEVAR_READONLY);
        break;
    }

    case CON_MULSCALE:
        C_GetManyVars(4);
        return 0;

    case CON_DIST:
    case CON_LDIST:
    case CON_GETANGLE:
    case CON_GETINCANGLE:
        C_GetNextVarType(GAMEVAR_READONLY);
        C_GetNextVar();
        C_GetNextVar();
        return 0;

    case CON_FOR:  // special-purpose iteration
    {
        ofstype offset;
        instype *tscrptr;
        int32_t how;

        C_GetNextVarType(GAMEVAR_READONLY|GAMEVAR_SPECIAL);  // only simple vars allowed

        C_GetNextLabelName();
        how = hash_find(&h_iter, tlabel);
        if (how < 0)
        {
            C_CUSTOMERROR("unknown iteration type `%s'.", tlabel);
            return 1;
        }
        *g_scriptPtr++ = how;

        if (how <= ITER_DRAWNSPRITES) {}
        else C_GetNextVarType(0);

        offset = g_scriptPtr-script;
        g_scriptPtr++; //Leave a spot for the location to jump to after completion

        C_ParseCommand();

        tscrptr = (instype *)script+offset;
        *tscrptr = (g_scriptPtr-script)-offset;  // relative offset
        return 0;
    }

// *** if&while var&varvar
    case CON_IFVARL:
    case CON_IFVARLE:
    case CON_IFVARG:
    case CON_IFVARGE:
    case CON_IFVARE:
    case CON_IFVARN:
    case CON_IFVARAND:
    case CON_IFVAROR:
    case CON_IFVARXOR:
    case CON_IFVAREITHER:
    case CON_IFVARBOTH:
    case CON_WHILEVARN:
    case CON_WHILEVARL:
// ---
    case CON_IFVARVARL:
    case CON_IFVARVARLE:
    case CON_IFVARVARG:
    case CON_IFVARVARGE:
    case CON_IFVARVARE:
    case CON_IFVARVARN:
    case CON_IFVARVARAND:
    case CON_IFVARVAROR:
    case CON_IFVARVARXOR:
    case CON_IFVARVAREITHER:
    case CON_IFVARVARBOTH:
    case CON_WHILEVARVARN:
    case CON_WHILEVARVARL:
// ---
    case CON_IFHITKEY:
    case CON_IFHOLDKEY:
    case CON_IFRND:
// vvv if* using current sprite
    case CON_IFANGDIFFL:
    case CON_IFSPRITEPAL:
    case CON_IFACTOR:
    case CON_IFSOUND:
    case CON_IFPDISTL:
    case CON_IFPDISTG:
///    case CON_IFGAPZL:
///    case CON_IFFLOORDISTL:
///    case CON_IFCEILINGDISTL:
// ---
    case CON_IFINSIDE:
// ---
    case CON_IFEITHERALT:
    case CON_IFEITHERCTRL:
    case CON_IFEITHERSHIFT:
    case CON_IFAWAYFROMWALL:
    case CON_IFCANSEE:
    case CON_IFONWATER:
    case CON_IFINWATER:
    case CON_IFOUTSIDE:
///    case CON_IFHITSPACE:
///    case CON_IFINSPACE:
///    case CON_IFINOUTERSPACE:
///    case CON_IFCANSEETARGET:
    case CON_IFNOSOUNDS:
    {
        ofstype offset;
        ofstype lastScriptOfs = (g_scriptPtr-script-1);
        instype *tscrptr;

        cs.ifElseAborted = 0;

        if (tw<=CON_WHILEVARL)  // careful! check this against order in m32def.h!
        {
            C_GetNextVar();
            C_GetNextValue(LABEL_DEFINE); // the number to check against...
        }
        else if (tw<=CON_WHILEVARVARL)
        {
            instype *inst = (g_scriptPtr-1);
            const char *otextptr;

            C_GetNextVar();

            otextptr = textptr;

            g_wasConstant = 0;
            C_GetNextVar();

            if (!g_numCompilerErrors && g_wasConstant)
            {
                textptr = otextptr;
                g_scriptPtr--;
                *inst -= (CON_IFVARVARL - CON_IFVARL);
                C_GetNextValue(LABEL_DEFINE);
            }
        }
        else if (tw<=CON_IFPDISTG)
///            C_GetNextValue(LABEL_DEFINE);
            C_GetNextVar();
        else if (tw<=CON_IFINSIDE)
            C_GetManyVars(3);
        // else {}

        if (C_CheckMalformedBranch(lastScriptOfs))
            return 0;

        offset = (g_scriptPtr-script);
        g_scriptPtr++; //Leave a spot for the fail location

        C_ParseCommand();

        if (C_CheckEmptyBranch(tw, lastScriptOfs))
            return 0;

        tscrptr = (instype *)script+offset;
        *tscrptr = (g_scriptPtr-script)-offset;  // relative offset

        if (tw != CON_WHILEVARN && tw != CON_WHILEVARVARN)
        {
            j = C_GetKeyword();

            if (j == CON_ELSE || j == CON_LEFTBRACE)
                cs.checkingIfElse++;
        }

        return 0;
    }

// *** BUILD functions
    case CON_TDUPSPRITE:
        if (cs.currentEvent>=0 && cs.currentEvent != EVENT_ANALYZESPRITES)
        {
            C_ReportError(WARNING_OUTSIDEDRAWSPRITE);
            g_numCompilerWarnings++;
        }
    case CON_RESETKEY:
    case CON_INSERTSPRITE:
    case CON_DUPSPRITE:
    case CON_DELETESPRITE:
        C_GetNextVar();
        break;

    case CON_LASTWALL:
        C_GetNextVar();
        C_GetNextVarType(GAMEVAR_READONLY);
        break;

    case CON_UPDATECURSECTNUM:
        return 0;

    case CON_UPDATESECTOR:
    case CON_UPDATESECTORZ:
        C_GetManyVars(2);
        if (tw==CON_UPDATESECTORZ)
            C_GetNextVar();
        C_GetNextVarType(GAMEVAR_READONLY);
        break;

    case CON_GETZRANGE:
        C_GetManyVars(4);
        C_GetManyVarsType(GAMEVAR_READONLY,4);
        C_GetManyVars(2);
        break;

    case CON_HITSCAN:
    case CON_CANSEE:
        // get the ID of the DEF
        C_GetManyVars(tw==CON_CANSEE?8:7);
        C_GetManyVarsType(GAMEVAR_READONLY,tw==CON_CANSEE?1:6);
        if (tw==CON_HITSCAN) C_GetNextVar();
        break;
    case CON_CANSEESPR:
        C_GetManyVars(2);
        C_GetNextVarType(GAMEVAR_READONLY);
        break;

    case CON_NEARTAG:
    case CON_ROTATEPOINT:
        C_GetManyVars(5);
        C_GetManyVarsType(GAMEVAR_READONLY,2);
        if (tw == CON_NEARTAG)
        {
            C_GetManyVarsType(GAMEVAR_READONLY,2);
            C_GetManyVars(2);
        }
        break;
    case CON_DRAGPOINT:
        C_GetManyVars(3);
        return 0;

    case CON_GETFLORZOFSLOPE:
    case CON_GETCEILZOFSLOPE:
        C_GetManyVars(3);
        C_GetNextVarType(GAMEVAR_READONLY);
        return 0;

    case CON_ALIGNCEILSLOPE:
    case CON_ALIGNFLORSLOPE:
    case CON_BSETSPRITE:  // was CON_SETSPRITE
        C_GetManyVars(4);
        break;

    case CON_SETFIRSTWALL:
    case CON_CHANGESPRITESTAT:
    case CON_CHANGESPRITESECT:
    case CON_HEADSPRITESTAT:
    case CON_PREVSPRITESTAT:
    case CON_NEXTSPRITESTAT:
    case CON_HEADSPRITESECT:
    case CON_PREVSPRITESECT:
    case CON_NEXTSPRITESECT:
        C_GetManyVars(2);
        return 0;

    case CON_SECTOROFWALL:
        C_GetNextVarType(GAMEVAR_READONLY);
        C_GetNextVar();
        return 0;

    case CON_FIXREPEATS:
        C_GetNextVar();
        return 0;

// *** stuff
    case CON_ADDLOGVAR:
        // syntax: addlogvar <var>
        // prints the line number in the log file.
        C_GetNextVar();
        return 0;

    case CON_ADDLOG:
        // syntax: addlog
        // prints the line number in the log file.
        return 0;
    case CON_DEBUG:
        C_GetNextValue(LABEL_DEFINE);
        return 0;

// *** strings
    case CON_DEFINEQUOTE:
    case CON_REDEFINEQUOTE:
        if (tw == CON_DEFINEQUOTE)
            g_scriptPtr--;

        C_GetNextValue(LABEL_DEFINE);

        k = *(g_scriptPtr-1);

        if (k<0 || k >= MAXQUOTES)
        {
            C_CUSTOMERROR("quote number out of range (0 to %d).", MAXQUOTES-1);
            k = MAXQUOTES;
        }

        if (ScriptQuotes[k] == NULL)
            ScriptQuotes[k] = Bcalloc(MAXQUOTELEN,sizeof(uint8_t));

        if (!ScriptQuotes[k])
        {
            Bsprintf(tempbuf,"Failed allocating %" PRIdPTR " byte quote text buffer.",sizeof(uint8_t) * MAXQUOTELEN);
            g_numCompilerErrors++;
            return 1;
        }

        if (tw == CON_DEFINEQUOTE)
            g_scriptPtr--;

        while (*textptr == ' ' || *textptr == '\t')
            textptr++;

        if (tw == CON_REDEFINEQUOTE)
        {
            if (ScriptQuoteRedefinitions[g_numQuoteRedefinitions] == NULL)
                ScriptQuoteRedefinitions[g_numQuoteRedefinitions] = Bcalloc(MAXQUOTELEN,sizeof(uint8_t));
            if (!ScriptQuoteRedefinitions[g_numQuoteRedefinitions])
            {
                Bsprintf(tempbuf,"Failed allocating %" PRIdPTR " byte quote text buffer.",sizeof(uint8_t) * MAXQUOTELEN);
                g_numCompilerErrors++;
                return 1;
            }
        }

        i = 0;
        while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        {
//            if (*textptr == '%' && *(textptr+1) == 's')
//            {
//                initprintf("%s:%d: error: quote text contains string identifier.\n",g_szScriptFileName,g_lineNumber);
//                g_numCompilerErrors++;
//                while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0) textptr++;
//                break;
//            }
            if (tw == CON_DEFINEQUOTE)
                *(ScriptQuotes[k]+i) = *textptr;
            else
                *(ScriptQuoteRedefinitions[g_numQuoteRedefinitions]+i) = *textptr;

            textptr++;
            i++;

            if (i >= MAXQUOTELEN-1)
            {
                C_CUSTOMWARNING("truncating quote text to %d characters.", MAXQUOTELEN-1);
                while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
                    textptr++;
                break;
            }
        }

        if (tw == CON_DEFINEQUOTE)
            *(ScriptQuotes[k]+i) = '\0';
        else
        {
            *(ScriptQuoteRedefinitions[g_numQuoteRedefinitions]+i) = '\0';
            *g_scriptPtr++ = g_numQuoteRedefinitions;
            g_numQuoteRedefinitions++;
        }
        return 0;

    case CON_PRINT:
    case CON_QUOTE:
    case CON_ERRORINS:
    case CON_PRINTMESSAGE16:
        C_GetNextVar();
        return 0;
    case CON_PRINTMESSAGE256:
        C_GetManyVars(3);
        return 0;
    case CON_PRINTEXT256:
    case CON_PRINTEXT16:
        C_GetManyVars(6);
        return 0;

    case CON_GETNUMBER16:
    case CON_GETNUMBER256:
        C_GetNextVarType(GAMEVAR_READONLY);
        C_GetManyVars(2);
        return 0;

    case CON_QSPRINTF:
        C_GetManyVars(2);

        j = 0;
        while (C_GetKeyword() == -1 && j < 32)
            C_GetNextVar(), j++;

        *g_scriptPtr++ = -1; //CON_NULLOP + (g_lineNumber<<12);
        return 0;

    case CON_QSTRCAT:
    case CON_QSTRCPY:
    case CON_QSTRLEN:
#if 0
    case CON_QGETSYSSTR:
        C_GetManyVars(2);
        return 0;
#endif
    case CON_QSTRNCAT:
        C_GetManyVars(3);
        return 0;
    case CON_QSUBSTR:
        C_GetManyVars(4);
        return 0;

// *** findnear*
    case CON_FINDNEARSPRITE:
    case CON_FINDNEARSPRITE3D:
    case CON_FINDNEARSPRITEZ:
    {
        // syntax findnearactor <type> <maxdist> <getvar>
        // gets the sprite ID of the nearest actor within max dist
        // that is of <type> into <getvar>
        // -1 for none found

        C_GetNextValue(LABEL_DEFINE); // get <type>
        C_GetNextValue(LABEL_DEFINE); // get maxdist

        if (tw==CON_FINDNEARSPRITEZ)
            C_GetNextValue(LABEL_DEFINE);

        // target var
        // get the ID of the DEF
        C_GetNextVarType(GAMEVAR_READONLY);
        break;
    }

    case CON_FINDNEARSPRITEVAR:
    case CON_FINDNEARSPRITE3DVAR:
    case CON_FINDNEARSPRITEZVAR:
    {
        C_GetNextValue(LABEL_DEFINE); // get <type>

        // get the ID of the DEF
        C_GetNextVar();

        if (tw==CON_FINDNEARSPRITEZVAR)
            C_GetNextVar();

        // target var
        // get the ID of the DEF
        C_GetNextVarType(GAMEVAR_READONLY);
        break;
    }


    case CON_GETTICKS:
        C_GetNextVarType(GAMEVAR_READONLY);
        return 0;

    case CON_GETTIMEDATE:
        C_GetManyVarsType(GAMEVAR_READONLY,8);
        break;

    case CON_SETASPECT:
        C_GetNextVar();        // get the ID of the DEF
        C_GetNextVar();        // get the ID of the DEF
        return 0;

    case CON_SPGETLOTAG:
    case CON_SPGETHITAG:
    case CON_SECTGETLOTAG:
    case CON_SECTGETHITAG:
    case CON_GETTEXTUREFLOOR:
    case CON_GETTEXTURECEILING:
    case CON_STOPALLSOUNDS:
        // no paramaters...
        return 0;

    case CON_SETI:
        C_GetNextVar();
        return 0;

    case CON_SIZEAT:
        C_GetManyVars(2);
//        C_GetNextValue(LABEL_DEFINE);
//        C_GetNextValue(LABEL_DEFINE);
        break;

///    case CON_ANGOFF:

    case CON_GETSOUNDFLAGS:
        C_GetNextVar();
        C_GetNextVarType(GAMEVAR_READONLY);
        break;

    case CON_SOUNDVAR:
    case CON_STOPSOUNDVAR:
    case CON_SOUNDONCEVAR:
    case CON_GLOBALSOUNDVAR:
    case CON_CSTATOR:
    case CON_SPRITEPAL:
    case CON_CACTOR:
    case CON_CLIPDIST:
        C_GetNextVar();
        return 0;

    case CON_CSTAT:
        C_GetNextValue(LABEL_DEFINE);
        if (*(g_scriptPtr-1) == 32767)
        {
            *(g_scriptPtr-1) = 32768;
            C_CUSTOMWARNING("tried to set cstat 32767, using 32768 instead.");
        }
        else if ((*(g_scriptPtr-1) & 32) && (*(g_scriptPtr-1) & 16))
        {
            i = *(g_scriptPtr-1);
            *(g_scriptPtr-1) ^= 48;
            C_CUSTOMWARNING("tried to set cstat %d, using %d instead.", i, *(g_scriptPtr-1));
        }
        return 0;

    case CON_DRAWLINE16:
    case CON_DRAWLINE16B:
    case CON_DRAWCIRCLE16:
    case CON_DRAWCIRCLE16B:
        if (cs.parsingEventOfs < 0 && cs.currentStateIdx < 0)
        {
            C_ReportError(ERROR_EVENTONLY);
            g_numCompilerErrors++;
        }
        C_GetManyVars((tw==CON_DRAWLINE16||tw==CON_DRAWLINE16B) ? 5 : 4);
        break;

    case CON_ROTATESPRITE16:
    case CON_ROTATESPRITE:
        if (cs.parsingEventOfs < 0 && cs.currentStateIdx < 0)
        {
            C_ReportError(ERROR_EVENTONLY);
            g_numCompilerErrors++;
        }
        // syntax:
        // int32_t x, int32_t y, int32_t z, short a, short tilenum, int8_t shade, char orientation, x1, y1, x2, y2
        // myospal adds char pal
        C_GetManyVars(12);        // get the ID of the DEFs
        break;

    case CON_SETGAMEPALETTE:
        C_GetNextVar();
        return 0;
    }
    return 0;
}

/* Anything added with C_AddDefinition() cannot be overwritten in the CONs */

static void C_AddDefinition(const char *lLabel,int32_t lValue, uint8_t lType)
{
    Bstrcpy(label+(g_numLabels*MAXLABELLEN), lLabel);
    hash_add(&h_labels, label+(g_numLabels*MAXLABELLEN), g_numLabels);
    labeltype[g_numLabels] = lType;
    labelval[g_numLabels++] = lValue;
    g_numDefaultLabels++;
}

static void C_AddDefaultDefinitions(void)
{
    // events must come first and in correct order
    C_AddDefinition("EVENT_ENTER3DMODE", EVENT_ENTER3DMODE, LABEL_EVENT);
    C_AddDefinition("EVENT_ANALYZESPRITES", EVENT_ANALYZESPRITES, LABEL_EVENT);
    C_AddDefinition("EVENT_INSERTSPRITE2D", EVENT_INSERTSPRITE2D, LABEL_EVENT);
    C_AddDefinition("EVENT_INSERTSPRITE3D", EVENT_INSERTSPRITE3D, LABEL_EVENT);
    C_AddDefinition("EVENT_DRAW2DSCREEN", EVENT_DRAW2DSCREEN, LABEL_EVENT);
    C_AddDefinition("EVENT_KEYS2D", EVENT_KEYS2D, LABEL_EVENT);
    C_AddDefinition("EVENT_KEYS3D", EVENT_KEYS3D, LABEL_EVENT);
    C_AddDefinition("EVENT_PREKEYS2D", EVENT_PREKEYS2D, LABEL_EVENT);
    C_AddDefinition("EVENT_PREKEYS3D", EVENT_PREKEYS3D, LABEL_EVENT);

    C_AddDefinition("CLIPMASK0", CLIPMASK0, LABEL_DEFINE);
    C_AddDefinition("CLIPMASK1", CLIPMASK1, LABEL_DEFINE);

    C_AddDefinition("MAXSPRITES", MAXSPRITES, LABEL_DEFINE);
    C_AddDefinition("MAXSECTORS", MAXSECTORS, LABEL_DEFINE);
    C_AddDefinition("MAXWALLS", MAXWALLS, LABEL_DEFINE);
    C_AddDefinition("MAXTILES", MAXTILES, LABEL_DEFINE);
    C_AddDefinition("MAXSTATUS", MAXSTATUS, LABEL_DEFINE);
    C_AddDefinition("MAXSOUNDS", MAXSOUNDS, LABEL_DEFINE);

// keys
    C_AddDefinition("KEY_SPACE", KEYSC_SPACE, LABEL_DEFINE);

    C_AddDefinition("KEY_A", KEYSC_A, LABEL_DEFINE);
    C_AddDefinition("KEY_B", KEYSC_B, LABEL_DEFINE);
    C_AddDefinition("KEY_C", KEYSC_C, LABEL_DEFINE);
    C_AddDefinition("KEY_D", KEYSC_D, LABEL_DEFINE);
    C_AddDefinition("KEY_E", KEYSC_E, LABEL_DEFINE);
    C_AddDefinition("KEY_F", KEYSC_F, LABEL_DEFINE);
    C_AddDefinition("KEY_G", KEYSC_G, LABEL_DEFINE);
    C_AddDefinition("KEY_H", KEYSC_H, LABEL_DEFINE);
    C_AddDefinition("KEY_I", KEYSC_I, LABEL_DEFINE);
    C_AddDefinition("KEY_J", KEYSC_J, LABEL_DEFINE);
    C_AddDefinition("KEY_K", KEYSC_K, LABEL_DEFINE);
    C_AddDefinition("KEY_L", KEYSC_L, LABEL_DEFINE);
    C_AddDefinition("KEY_M", KEYSC_M, LABEL_DEFINE);
    C_AddDefinition("KEY_N", KEYSC_N, LABEL_DEFINE);
    C_AddDefinition("KEY_O", KEYSC_O, LABEL_DEFINE);
    C_AddDefinition("KEY_P", KEYSC_P, LABEL_DEFINE);
    C_AddDefinition("KEY_Q", KEYSC_Q, LABEL_DEFINE);
    C_AddDefinition("KEY_R", KEYSC_R, LABEL_DEFINE);
    C_AddDefinition("KEY_S", KEYSC_S, LABEL_DEFINE);
    C_AddDefinition("KEY_T", KEYSC_T, LABEL_DEFINE);
    C_AddDefinition("KEY_U", KEYSC_U, LABEL_DEFINE);
    C_AddDefinition("KEY_V", KEYSC_V, LABEL_DEFINE);
    C_AddDefinition("KEY_W", KEYSC_W, LABEL_DEFINE);
    C_AddDefinition("KEY_X", KEYSC_X, LABEL_DEFINE);
    C_AddDefinition("KEY_Y", KEYSC_Y, LABEL_DEFINE);
    C_AddDefinition("KEY_Z", KEYSC_Z, LABEL_DEFINE);

    C_AddDefinition("KEY_ENTER", KEYSC_ENTER, LABEL_DEFINE);
    C_AddDefinition("KEY_BS", KEYSC_BS, LABEL_DEFINE);
    C_AddDefinition("KEY_TAB", KEYSC_TAB, LABEL_DEFINE);

    C_AddDefinition("KEY_0", KEYSC_0, LABEL_DEFINE);
    C_AddDefinition("KEY_1", KEYSC_1, LABEL_DEFINE);
    C_AddDefinition("KEY_2", KEYSC_2, LABEL_DEFINE);
    C_AddDefinition("KEY_3", KEYSC_3, LABEL_DEFINE);
    C_AddDefinition("KEY_4", KEYSC_4, LABEL_DEFINE);
    C_AddDefinition("KEY_5", KEYSC_5, LABEL_DEFINE);
    C_AddDefinition("KEY_6", KEYSC_6, LABEL_DEFINE);
    C_AddDefinition("KEY_7", KEYSC_7, LABEL_DEFINE);
    C_AddDefinition("KEY_8", KEYSC_8, LABEL_DEFINE);
    C_AddDefinition("KEY_9", KEYSC_9, LABEL_DEFINE);

    C_AddDefinition("KEY_DASH", KEYSC_DASH, LABEL_DEFINE);
    C_AddDefinition("KEY_EQUAL", KEYSC_EQUAL, LABEL_DEFINE);
    C_AddDefinition("KEY_LBRACK", KEYSC_LBRACK, LABEL_DEFINE);
    C_AddDefinition("KEY_RBRACK", KEYSC_RBRACK, LABEL_DEFINE);
    C_AddDefinition("KEY_SEMI", KEYSC_SEMI, LABEL_DEFINE);
    C_AddDefinition("KEY_QUOTE", KEYSC_QUOTE, LABEL_DEFINE);
    C_AddDefinition("KEY_BQUOTE", KEYSC_BQUOTE, LABEL_DEFINE);
    C_AddDefinition("KEY_BSLASH", KEYSC_BSLASH, LABEL_DEFINE);
    C_AddDefinition("KEY_COMMA", KEYSC_COMMA, LABEL_DEFINE);
    C_AddDefinition("KEY_PERIOD", KEYSC_PERIOD, LABEL_DEFINE);
    C_AddDefinition("KEY_SLASH", KEYSC_SLASH, LABEL_DEFINE);

    C_AddDefinition("KEY_LALT", KEYSC_LALT, LABEL_DEFINE);
    C_AddDefinition("KEY_LCTRL", KEYSC_LCTRL, LABEL_DEFINE);
    C_AddDefinition("KEY_LSHIFT", KEYSC_LSHIFT, LABEL_DEFINE);
    C_AddDefinition("KEY_RALT", KEYSC_RALT, LABEL_DEFINE);
    C_AddDefinition("KEY_RCTRL", KEYSC_RCTRL, LABEL_DEFINE);
    C_AddDefinition("KEY_RSHIFT", KEYSC_RSHIFT, LABEL_DEFINE);

// some aliases...
    C_AddDefinition("KEY_KP7", KEYSC_gHOME, LABEL_DEFINE);
    C_AddDefinition("KEY_KP8", KEYSC_gUP, LABEL_DEFINE);
    C_AddDefinition("KEY_KP9", KEYSC_gPGUP, LABEL_DEFINE);
    C_AddDefinition("KEY_KP4", KEYSC_gLEFT, LABEL_DEFINE);
    C_AddDefinition("KEY_KP5", KEYSC_gKP5, LABEL_DEFINE);
    C_AddDefinition("KEY_KP6", KEYSC_gRIGHT, LABEL_DEFINE);
    C_AddDefinition("KEY_KP1", KEYSC_gEND, LABEL_DEFINE);
    C_AddDefinition("KEY_KP2", KEYSC_gDOWN, LABEL_DEFINE);
    C_AddDefinition("KEY_KP3", KEYSC_gPGDN, LABEL_DEFINE);
    C_AddDefinition("KEY_KP0", KEYSC_gINS, LABEL_DEFINE);
    C_AddDefinition("KEY_KPCOMMA", KEYSC_gDEL, LABEL_DEFINE);

    C_AddDefinition("KEY_gDEL", KEYSC_gDEL, LABEL_DEFINE);
    C_AddDefinition("KEY_gDOWN", KEYSC_gDOWN, LABEL_DEFINE);
    C_AddDefinition("KEY_gEND", KEYSC_gEND, LABEL_DEFINE);
    C_AddDefinition("KEY_gHOME", KEYSC_gHOME, LABEL_DEFINE);
    C_AddDefinition("KEY_gINS", KEYSC_gINS, LABEL_DEFINE);
    C_AddDefinition("KEY_gKP5", KEYSC_gKP5, LABEL_DEFINE);
    C_AddDefinition("KEY_gLEFT", KEYSC_gLEFT, LABEL_DEFINE);
    C_AddDefinition("KEY_gMINUS", KEYSC_gMINUS, LABEL_DEFINE);
    C_AddDefinition("KEY_gPGDN", KEYSC_gPGDN, LABEL_DEFINE);
    C_AddDefinition("KEY_gPGUP", KEYSC_gPGUP, LABEL_DEFINE);
    C_AddDefinition("KEY_gPLUS", KEYSC_gPLUS, LABEL_DEFINE);
    C_AddDefinition("KEY_gRIGHT", KEYSC_gRIGHT, LABEL_DEFINE);
    C_AddDefinition("KEY_gSLASH", KEYSC_gSLASH, LABEL_DEFINE);
    C_AddDefinition("KEY_gSTAR", KEYSC_gSTAR, LABEL_DEFINE);
    C_AddDefinition("KEY_gUP", KEYSC_gUP, LABEL_DEFINE);

    C_AddDefinition("KEY_HOME", KEYSC_HOME, LABEL_DEFINE);
    C_AddDefinition("KEY_UP", KEYSC_UP, LABEL_DEFINE);
    C_AddDefinition("KEY_PGUP", KEYSC_PGUP, LABEL_DEFINE);
    C_AddDefinition("KEY_LEFT", KEYSC_LEFT, LABEL_DEFINE);
    C_AddDefinition("KEY_RIGHT", KEYSC_RIGHT, LABEL_DEFINE);
    C_AddDefinition("KEY_END", KEYSC_END, LABEL_DEFINE);
    C_AddDefinition("KEY_DOWN", KEYSC_DOWN, LABEL_DEFINE);
    C_AddDefinition("KEY_PGDN", KEYSC_PGDN, LABEL_DEFINE);
    C_AddDefinition("KEY_INSERT", KEYSC_INSERT, LABEL_DEFINE);
    C_AddDefinition("KEY_DELETE", KEYSC_DELETE, LABEL_DEFINE);
// end keys

//    C_AddDefinition("STR_MAPFILENAME",STR_MAPFILENAME, LABEL_DEFINE);
//    C_AddDefinition("STR_VERSION",STR_VERSION, LABEL_DEFINE);

    C_AddDefinition("NO",0, LABEL_DEFINE);
    C_AddDefinition("COLOR_WHITE",31, LABEL_DEFINE);
}

void C_CompilationInfo(void)
{
    int32_t j, k=0;
    initprintf("Compiled code info: (size=%ld*%d bytes)\n",
               (unsigned)(g_scriptPtr-script), sizeof(instype));
    initprintf("  %d/%d user labels, %d/65536 indirect constants,\n",
               g_numLabels-g_numDefaultLabels, 65536-g_numDefaultLabels,
               g_numSavedConstants);
    initprintf("  %d/%d user variables, %d/%d user arrays\n",
               g_gameVarCount-g_systemVarCount, MAXGAMEVARS-g_systemVarCount,
               g_gameArrayCount-g_systemArrayCount, MAXGAMEARRAYS-g_systemArrayCount);
    for (j=0; j<MAXEVENTS; j++)
        if (aEventOffsets[j] >= 0)
            k++;
    initprintf("  %d states, %d/%d defined events\n", g_stateCount, k,MAXEVENTS);

    for (k=0, j=MAXQUOTES-1; j>=0; j--)
        if (ScriptQuotes[j])
            k++;
    if (k || g_numQuoteRedefinitions)
        initprintf("  %d/%d quotes, %d/%d quote redefinitions\n", k,MAXQUOTES, g_numQuoteRedefinitions,MAXQUOTES);
}

void C_Compile(const char *filenameortext, int32_t isfilename)
{
    char *mptr = NULL;
    static char firstime=1;
    int32_t i,j;
    int32_t fs,fp;
    int32_t startcompiletime;
    instype* oscriptPtr;
    int32_t ostateCount = g_stateCount;

    if (firstime)
    {
        label = Bmalloc(label_allocsize * MAXLABELLEN * sizeof(char));
        labelval = Bmalloc(label_allocsize * sizeof(int32_t));
        labeltype = Bmalloc(label_allocsize * sizeof(uint8_t));

        constants = Bmalloc(constants_allocsize * sizeof(int32_t));

        statesinfo = Bmalloc(statesinfo_allocsize * sizeof(statesinfo_t));

        for (i=0; i<MAXEVENTS; i++)
        {
            aEventOffsets[i] = -1;
            aEventEnabled[i] = 0;
        }

        C_InitHashes();
        Gv_Init();
        C_AddDefaultDefinitions();

        script = Bcalloc(g_scriptSize, sizeof(instype));
        //        initprintf("script: %d\n",script);
        if (!script || !label || !labelval || !labeltype || !constants)
        {
            initprintf("C_Compile(): ERROR: out of memory!\n");
            g_numCompilerErrors++;
            return;
        }

        if ((sizeof(keyw)/sizeof(keyw[0]))-1 != CON_END)
            initprintf("INTERNAL WARNING: keyw[] and CON_END don't match!\n");

        g_scriptPtr = script+1;

        firstime = 0;
    }

    if (isfilename)
    {
        fs = Bstrlen(filenameortext);
        mptr = Bmalloc(fs+1+4);
        if (!mptr)
        {
            initprintf("C_Compile(): ERROR: out of memory!\n");
            g_numCompilerErrors++;
            return;
        }
        Bmemcpy(mptr, filenameortext, fs+1);

        fp = kopen4load(mptr, 0 /*g_loadFromGroupOnly*/);
        if (fp == -1) // JBF: was 0
        {
            Bstrcat(&mptr[fs], ".m32");
            fp = kopen4load(mptr, 0 /*g_loadFromGroupOnly*/);
            if (fp == -1)
            {
                initprintf("M32 file `%s' not found.\n", mptr);
                Bfree(mptr);
                //g_loadFromGroupOnly = 1;
                return;
            }
        }

        fs = kfilelength(fp);
        initprintf("--- Compiling: %s (%d bytes)\n",mptr,fs);
        Bstrcpy(g_szScriptFileName, mptr);   // JBF 20031130: Store currently compiling file name
        Bfree(mptr);
    }
    else
    {
        Bsprintf(g_szScriptFileName, "(console)");
//        fs = Bstrlen(filenameortext);
//        initprintf("Compiling: (from console) (%d bytes)\n",fs);
    }

//    flushlogwindow = 0;

    startcompiletime = getticks();

    if (isfilename)
    {
        mptr = (char *)Bmalloc(fs+1);
        if (!mptr)
        {
            initprintf("Failed allocating %d byte CON text buffer.\n", fs+1);
            return;
        }

        mptr[fs] = 0;

        kread(fp, mptr, fs);
        kclose(fp);
        textptr = (char *)mptr;
    }
    else
        textptr = filenameortext;

//////
    g_numCompilerWarnings = g_numCompilerErrors = 0;

    g_lineNumber = 1;
    g_totalLines = 0;
//////

    oscriptPtr = g_scriptPtr;
    g_didDefineSomething = 0;

    Bmemcpy(&cs, &cs_default, sizeof(compilerstate_t));

    while (C_ParseCommand() == 0);

//    flushlogwindow = 1;

    if (g_numCompilerErrors >= ABORTERRCNT)
        initprintf("fatal error: too many errors: Aborted\n");

    //*script = g_scriptPtr-script;

    if (mptr)
        Bfree(mptr);

    if (g_stateCount > ostateCount)
    {
        for (i=0; i<g_gameVarCount; i++)
            if (aGameVars[i].dwFlags & GAMEVAR_PERBLOCK)
            {
                if (aGameVars[i].val.plValues)
                {
                    aGameVars[i].val.plValues = Brealloc(aGameVars[i].val.plValues, (1+MAXEVENTS+g_stateCount)*sizeof(int32_t));
                    for (j=ostateCount; j<g_stateCount; j++)
                        aGameVars[i].val.plValues[1+MAXEVENTS+j] = aGameVars[i].lDefault;
                }
                else
                {
                    aGameVars[i].val.plValues = Bmalloc((1+MAXEVENTS+g_stateCount)*sizeof(int32_t));
                    for (j=0; j<(1+MAXEVENTS+g_stateCount); j++)
                        aGameVars[i].val.plValues[j] = aGameVars[i].lDefault;
                }
            }
    }

    if (g_numCompilerErrors)
    {
        if (!g_didDefineSomething)
            g_scriptPtr = oscriptPtr;
    }
    else
    {
        g_totalLines += g_lineNumber;
//        C_SetScriptSize(g_scriptPtr-script+8);
        if (isfilename)
        {
            int32_t ct = getticks() - startcompiletime;
            if (ct > 50)
                initprintf("Script compiled in %dms\n", ct);
            C_CompilationInfo();
        }
///        for (i=MAXQUOTES-1; i>=0; i--)
///            if (ScriptQuotes[i] == NULL)
///                ScriptQuotes[i] = Bcalloc(MAXQUOTELEN,sizeof(uint8_t));
    }

    if (g_numCompilerErrors)
    {
        initprintf("--- Found %d errors", g_numCompilerErrors);
        if (g_numCompilerWarnings)
            initprintf(", %d warnings.\n", g_numCompilerWarnings);
        else
            initprintf(".\n");
    }
    else if (g_numCompilerWarnings)
        initprintf("--- Found %d warnings.\n", g_numCompilerWarnings);
}

void C_ReportError(int32_t iError)
{
    if (Bstrcmp(g_szCurrentBlockName, g_szLastBlockName))
    {
        if (cs.parsingEventOfs >= 0 || cs.currentStateIdx >= 0)
            initprintf("%s: In %s `%s':\n",g_szScriptFileName, cs.parsingEventOfs >= 0 ? "event":"state", g_szCurrentBlockName);
        else
            initprintf("%s: At top level:\n", g_szScriptFileName);

        Bstrcpy(g_szLastBlockName, g_szCurrentBlockName);
    }

    switch (iError)
    {
    case ERROR_CLOSEBRACKET:
        initprintf("%s:%d: error: found more `}' than `{' before `%s'.\n",
                   g_szScriptFileName, g_lineNumber, tempbuf);
        break;
    case ERROR_EVENTONLY:
        initprintf("%s:%d: error: `%s' only valid during events.\n",
                   g_szScriptFileName, g_lineNumber, tempbuf);
        break;
//    case ERROR_EXCEEDSMAXTILES:
//        initprintf("%s:%d: error: `%s' value exceeds MAXTILES.  Maximum is %d.\n",
//                   g_szScriptFileName, g_lineNumber, tempbuf, MAXTILES-1);
//        break;
    case ERROR_EXPECTEDKEYWORD:
        initprintf("%s:%d: error: expected a keyword but found `%s'.\n",
                   g_szScriptFileName, g_lineNumber, tempbuf);
        break;
    case ERROR_FOUNDWITHIN:
        initprintf("%s:%d: error: found `%s' within %s.\n",
                   g_szScriptFileName, g_lineNumber, tempbuf, (cs.parsingEventOfs >= 0)?"an event":"a state");
        break;
    case ERROR_ISAKEYWORD:
        initprintf("%s:%d: error: symbol `%s' is a keyword.\n",
                   g_szScriptFileName, g_lineNumber, tlabel);
        break;
    case ERROR_NOENDSWITCH:
        initprintf("%s:%d: error: did not find `endswitch' before `%s'.\n",
                   g_szScriptFileName, g_lineNumber, tlabel);
        break;
    case ERROR_NOTAGAMEDEF:
        initprintf("%s:%d: error: symbol `%s' is not a game definition.\n",
                   g_szScriptFileName, g_lineNumber, tlabel);
        break;
    case ERROR_NOTAGAMEVAR:
        initprintf("%s:%d: error: symbol `%s' is not a game variable.\n",
                   g_szScriptFileName, g_lineNumber, tlabel);
        break;
    case ERROR_NOTAGAMEARRAY:
        initprintf("%s:%d: error: symbol `%s' is not a game array.\n",
                   g_szScriptFileName, g_lineNumber, tlabel);
        break;
    case ERROR_GAMEARRAYBNC:
        initprintf("%s:%d: error: square brackets for index of game array not closed, expected ] found %c\n",
                   g_szScriptFileName, g_lineNumber, *textptr);
        break;
    case ERROR_GAMEARRAYBNO:
        initprintf("%s:%d: error: square brackets for index of game array not opened, expected [ found %c\n",
                   g_szScriptFileName, g_lineNumber, *textptr);
        break;
    case ERROR_INVALIDARRAYWRITE:
        initprintf("%s:%d: error: arrays can only be written to using `setarray'\n",
                   g_szScriptFileName, g_lineNumber);
        break;
    case ERROR_EXPECTEDSIMPLEVAR:
        initprintf("%s:%d: error: expected a simple gamevar or a constant\n",
                   g_szScriptFileName, g_lineNumber);
        break;
    case ERROR_OPENBRACKET:
        initprintf("%s:%d: error: found more `{' than `}' before `%s'.\n",
                   g_szScriptFileName, g_lineNumber, tempbuf);
        break;
    case ERROR_PARAMUNDEFINED:
        initprintf("%s:%d: error: parameter `%s' is undefined.\n",
                   g_szScriptFileName, g_lineNumber, tempbuf);
        break;
    case ERROR_SYMBOLNOTRECOGNIZED:
        initprintf("%s:%d: error: symbol `%s' is not recognized.\n",
                   g_szScriptFileName, g_lineNumber, tlabel);
        break;
    case ERROR_SYNTAXERROR:
        initprintf("%s:%d: error: syntax error.\n",
                   g_szScriptFileName, g_lineNumber);
        break;
    case ERROR_VARREADONLY:
        initprintf("%s:%d: error: variable `%s' is read-only.\n",
                   g_szScriptFileName, g_lineNumber, tlabel);
        break;
    case ERROR_ARRAYREADONLY:
        initprintf("%s:%d: error: array `%s' is read-only.\n",
                   g_szScriptFileName, g_lineNumber, tlabel);
        break;
    case ERROR_VARTYPEMISMATCH:
        initprintf("%s:%d: error: variable `%s' is of the wrong type.\n",
                   g_szScriptFileName, g_lineNumber, tlabel);
        break;
    case ERROR_LABELINUSE:
        initprintf("%s:%d: error: label `%s' is already in use by a %s.\n",
                   g_szScriptFileName, g_lineNumber, tlabel, g_tw==CON_DEFSTATE?"define":"state");
        break;
//    case WARNING_BADGAMEVAR:
//        initprintf("%s:%ld: warning: variable `%s' should be either per-player OR per-actor, not both.\n",
//                   g_szScriptFileName, g_lineNumber, tlabel);
//        break;
    case WARNING_DUPLICATECASE:
        initprintf("%s:%ld: warning: duplicate case ignored.\n",
                   g_szScriptFileName, g_lineNumber);
        break;
    case WARNING_DUPLICATEDEFINITION:
        initprintf("%s:%d: warning: duplicate game definition `%s' ignored.\n",
                   g_szScriptFileName, g_lineNumber, tlabel);
        break;
    case WARNING_LABELSONLY:
        initprintf("%s:%d: warning: expected a label, found a constant.\n",
                   g_szScriptFileName, g_lineNumber);
        break;
    case WARNING_NAMEMATCHESVAR:
        initprintf("%s:%d: warning: symbol `%s' already used for game variable.\n",
                   g_szScriptFileName, g_lineNumber, tlabel);
//    case WARNING_CONSTANTBITSIZE:
//        initprintf("%s:%d: warning: constants where gamevars are expected save only 16 bits.\n",
//                   g_szScriptFileName,g_lineNumber);
//        break;
    case WARNING_OUTSIDEDRAWSPRITE:
        initprintf("%s:%d: warning: found `%s' outside of EVENT_ANALYZESPRITES\n",
                   g_szScriptFileName,g_lineNumber,tempbuf);
        break;
    }
}
