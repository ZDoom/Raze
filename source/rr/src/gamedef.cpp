//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

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

#include "ns.h"	// Must come before everything else!

#include "duke3d.h"
#include "dukerr/namesdyn.h"
#include "gamedef.h"
#include "gameexec.h"
#include "savegame.h"
#include "common.h"
#include "dukerr/common_game.h"
#include "cheats.h"
#include "m_argv.h"

#include "osd.h"
#include "m_crc32.h"
#include "printf.h"
#include "menu/menu.h"
#include "stringtable.h"
#include "mapinfo.h"

BEGIN_RR_NS

#define LINE_NUMBER (g_lineNumber << 12)

int32_t g_scriptVersion = 14; // 13 = 1.3D-style CON files, 14 = 1.4/1.5 style CON files

char g_scriptFileName[BMAX_PATH] = "(none)";  // file we're currently compiling

int32_t g_totalLines, g_lineNumber;
uint32_t g_scriptcrc;
char g_szBuf[1024];

static char g_szCurrentBlockName[256] = "(none)", g_szLastBlockName[256] = "NULL";
static int32_t g_checkingIfElse, g_processingState, g_lastKeyword = -1;

// The pointer to the start of the case table in a switch statement.
// First entry is 'default' code.
static intptr_t *g_caseScriptPtr;
static int32_t g_labelsOnly = 0;
static int32_t g_numBraces = 0;

static int32_t C_ParseCommand(int32_t loop);
static int32_t C_SetScriptSize(int32_t size);

static intptr_t apScriptGameEventEnd[MAXEVENTS_RR];
static intptr_t g_parsingActorPtr;
static intptr_t g_scriptEventOffset;
static char *textptr;

int32_t g_errorCnt,g_warningCnt;

static char *C_GetLabelType(int32_t type)
{
    int32_t i;
    char x[64];

    const char *LabelTypeText[] =
    {
        "define",
        "state",
        "actor",
        "action",
        "ai",
        "move"
    };

    x[0] = 0;
    for (i=0; i<6; i++)
    {
        if (!(type & (1<<i))) continue;
        if (x[0]) Bstrcat(x, " or ");
        Bstrcat(x, LabelTypeText[i]);
    }

    return Xstrdup(x);
}

static tokenmap_t const vm_keywords[] =
{
    { "action",                 CON_ACTION },
    { "actor",                  CON_ACTOR },
    { "addammo",                CON_ADDAMMO },
    { "addinventory",           CON_ADDINVENTORY },
    { "addkills",               CON_ADDKILLS },
    { "addlog",                 CON_ADDLOGVAR },
    { "addlogvar",              CON_ADDLOGVAR },
    { "addphealth",             CON_ADDPHEALTH },
    { "addstrength",            CON_ADDSTRENGTH },
    { "addvar",                 CON_ADDVAR },
    { "addvarvar",              CON_ADDVARVAR },
    { "addweapon",              CON_ADDWEAPON },
    { "ai",                     CON_AI },
    { "betaname",               CON_BETANAME },
    { "break",                  CON_BREAK },
    { "cactor",                 CON_CACTOR },
    { "clipdist",               CON_CLIPDIST },
    { "count",                  CON_COUNT },
    { "cstat",                  CON_CSTAT },
    { "cstator",                CON_CSTATOR },
    { "debris",                 CON_DEBRIS },
    { "debug",                  CON_DEBUG },
    { "define",                 CON_DEFINE },
    { "definelevelname",        CON_DEFINELEVELNAME },
    { "definequote",            CON_DEFINEQUOTE },
    { "defineskillname",        CON_DEFINESKILLNAME },
    { "definesound",            CON_DEFINESOUND },
    { "definevolumename",       CON_DEFINEVOLUMENAME },
    { "deploybias",             CON_DEPLOYBIAS },
    { "destroyit",              CON_DESTROYIT },
    { "else",                   CON_ELSE },
    { "enda",                   CON_ENDA },
    { "endevent",               CON_ENDEVENT },
    { "endofgame",              CON_ENDOFGAME },
    { "ends",                   CON_ENDS },
    { "fakebubba",              CON_FAKEBUBBA },
    { "fall",                   CON_FALL },
    { "feathers",               CON_MAIL },
    { "gamestartup",            CON_GAMESTARTUP },
    { "gamevar",                CON_GAMEVAR },
    { "garybanjo",              CON_GARYBANJO },
    { "getlastpal",             CON_GETLASTPAL },
    { "globalsound",            CON_GLOBALSOUND },
    { "guts",                   CON_GUTS },
    { "hitradius",              CON_HITRADIUS },
    { "ifaction",               CON_IFACTION },
    { "ifactioncount",          CON_IFACTIONCOUNT },
    { "ifactor",                CON_IFACTOR },
    { "ifactorhealthg",         CON_IFACTORHEALTHG },
    { "ifactorhealthl",         CON_IFACTORHEALTHL },
    { "ifactornotstayput",      CON_IFACTORNOTSTAYPUT },
    { "ifai",                   CON_IFAI },
    { "ifangdiffl",             CON_IFANGDIFFL },
    { "ifawayfromwall",         CON_IFAWAYFROMWALL },
    { "ifbulletnear",           CON_IFBULLETNEAR },
    { "ifcansee",               CON_IFCANSEE },
    { "ifcanseetarget",         CON_IFCANSEETARGET },
    { "ifcanshoottarget",       CON_IFCANSHOOTTARGET },
    { "ifceilingdistl",         CON_IFCEILINGDISTL },
    { "ifcount",                CON_IFCOUNT },
    { "ifdead",                 CON_IFDEAD },
    { "iffindnewspot",          CON_IFFINDNEWSPOT },
    { "iffloordistl",           CON_IFFLOORDISTL },
    { "ifgapzl",                CON_IFGAPZL },
    { "ifgotweaponce",          CON_IFGOTWEAPONCE },
    { "ifhitspace",             CON_IFHITSPACE },
    { "ifhittruck",             CON_IFHITTRUCK },
    { "ifhitweapon",            CON_IFHITWEAPON },
    { "ifinouterspace",         CON_IFINOUTERSPACE },
    { "ifinspace",              CON_IFINSPACE },
    { "ifinwater",              CON_IFINWATER },
    { "ifmotofast",             CON_IFMOTOFAST },
    { "ifmove",                 CON_IFMOVE },
    { "ifmultiplayer",          CON_IFMULTIPLAYER },
    { "ifnocover",              CON_IFNOCOVER },
    { "ifnosounds",             CON_IFNOSOUNDS },
    { "ifnotmoving",            CON_IFNOTMOVING },
    { "ifcoop",                 CON_IFCOOP },
    { "ifonboat",               CON_IFONBOAT },
    { "ifonmoto",               CON_IFONMOTO },
    { "ifonmud",                CON_IFONMUD },
    { "ifonwater",              CON_IFONWATER },
    { "ifoutside",              CON_IFOUTSIDE },
    { "ifp",                    CON_IFP },
    { "ifpdistg",               CON_IFPDISTG },
    { "ifpdistl",               CON_IFPDISTL },
    { "ifpdrunk",               CON_IFPDRUNK },
    { "ifphealthl",             CON_IFPHEALTHL },
    { "ifpinventory",           CON_IFPINVENTORY },
    { "ifrespawn",              CON_IFRESPAWN },
    { "ifrnd",                  CON_IFRND },
    { "ifsizedown",             CON_IFSIZEDOWN },
    { "ifsounddist",            CON_IFSOUNDDIST },
    { "ifsoundid",              CON_IFSOUNDID },
    { "ifspawnedby",            CON_IFSPAWNEDBY },
    { "ifspritepal",            CON_IFSPRITEPAL },
    { "ifsquished",             CON_IFSQUISHED },
    { "ifstrength",             CON_IFSTRENGTH },
    { "iftipcow",               CON_IFTIPCOW },
    { "ifpupwind",              CON_IFPUPWIND },
    { "ifvare",                 CON_IFVARE },
    { "ifvarg",                 CON_IFVARG },
    { "ifvarl",                 CON_IFVARL },
    { "ifvarvare",              CON_IFVARVARE },
    { "ifvarvarg",              CON_IFVARVARG },
    { "ifvarvarl",              CON_IFVARVARL },
    { "ifwasweapon",            CON_IFWASWEAPON },
    { "ifwind",                 CON_IFWIND },
    { "include",                CON_INCLUDE },
    { "isdrunk",                CON_ISDRUNK },
    { "iseat",                  CON_ISEAT },
    { "killit",                 CON_KILLIT },
    { "larrybird",              CON_LARRYBIRD },
    { "leavedroppings",         CON_LEAVEDROPPINGS },
    { "leavetrax",              CON_LEAVETRAX },
    { "lotsofglass",            CON_LOTSOFGLASS },
    { "mail",                   CON_MAIL },
    { "mamaend",                CON_MAMAEND },
    { "mamaquake",              CON_MAMAQUAKE },
    { "mamaspawn",              CON_MAMASPAWN },
    { "mamatrigger",            CON_MAMATRIGGER },
    { "mikesnd",                CON_MIKESND },
    { "money",                  CON_MONEY },
    { "motoloopsnd",            CON_MOTOLOOPSND },
    { "move",                   CON_MOVE },
    { "music",                  CON_MUSIC },
    { "newpic",                 CON_NEWPIC },
    { "nullop",                 CON_NULLOP },
    { "onevent",                CON_ONEVENT },
    { "operate",                CON_OPERATE },
    { "palfrom",                CON_PALFROM },
    { "paper",                  CON_PAPER },
    { "pkick",                  CON_PKICK },
    { "pstomp",                 CON_PSTOMP },
    { "quote",                  CON_QUOTE },
    { "resetactioncount",       CON_RESETACTIONCOUNT },
    { "resetcount",             CON_RESETCOUNT },
    { "resetplayer",            CON_RESETPLAYER },
    { "respawnhitag",           CON_RESPAWNHITAG },
    { "rndmove",                CON_RNDMOVE },
    { "setvar",                 CON_SETVAR },
    { "setvarvar",              CON_SETVARVAR },
    { "sizeat",                 CON_SIZEAT },
    { "sizeto",                 CON_SIZETO },
    { "slapplayer",             CON_SLAPPLAYER },
    { "sleeptime",              CON_SLEEPTIME },
    { "shoot",                  CON_SHOOT },
    { "smackbubba",             CON_SMACKBUBBA },
    { "smacksprite",            CON_SMACKSPRITE },
    { "sound",                  CON_SOUND },
    { "soundonce",              CON_SOUNDONCE },
    { "soundtag",               CON_SOUNDTAG },
    { "soundtagonce",           CON_SOUNDTAGONCE },
    { "spawn",                  CON_SPAWN },
    { "spritepal",              CON_SPRITEPAL },
    { "state",                  CON_STATE },
    { "stopsound",              CON_STOPSOUND },
    { "strafeleft",             CON_STRAFELEFT },
    { "straferight",            CON_STRAFERIGHT },
    { "strength",               CON_STRENGTH },
    { "tearitup",               CON_TEARITUP },
    { "tip",                    CON_TIP },
    { "tossweapon",             CON_TOSSWEAPON },
    { "useractor",              CON_USERACTOR },
    { "wackplayer",             CON_WACKPLAYER },
    { "{",                      CON_LEFTBRACE },
    { "}",                      CON_RIGHTBRACE },
};

static const vec2_t varvartable[] =
{
    { CON_IFVARVARE,         CON_IFVARE },
    { CON_IFVARVARG,         CON_IFVARG },
    { CON_IFVARVARL,         CON_IFVARL },

    { CON_ADDVARVAR,         CON_ADDVAR },
    { CON_SETVARVAR,         CON_SETVAR },
};

static inthashtable_t h_varvar = { NULL, INTHASH_SIZE(ARRAY_SIZE(varvartable)) };

static inthashtable_t *const inttables[] = {
    &h_varvar,
};

char const * VM_GetKeywordForID(int32_t id)
{
    // could be better but this is only called for diagnostics, ayy lmao
    for (tokenmap_t const & keyword : vm_keywords)
        if (keyword.val == id)
            return keyword.token;

    return "<invalid keyword>";
}

char *bitptr; // pointer to bitmap of which bytecode positions contain pointers
#define BITPTR_SET(x) (bitptr[(x)>>3] |= (1<<((x)&7)))
#define BITPTR_CLEAR(x) (bitptr[(x)>>3] &= ~(1<<((x)&7)))
#define BITPTR_IS_POINTER(x) (bitptr[(x)>>3] & (1<<((x) &7)))

hashtable_t h_gamevars = { MAXGAMEVARS >> 1, NULL };
hashtable_t h_labels   = { 11264>>1, NULL };

static hashtable_t h_keywords   = { CON_END>>1, NULL };;

static hashtable_t * const tables[] = {
    &h_labels, &h_keywords, &h_gamevars
};

static hashtable_t * const tables_free [] = {
    &h_labels, &h_keywords
};

#define STRUCT_HASH_SETUP(table, labels) do { for (i=0; labels[i].lId >= 0; i++) hash_add(&table, labels[i].name, i, 0); } while (0)

void C_InitHashes()
{
    for (auto table : tables)
        hash_init(table);

    for (auto table : inttables)
        inthash_init(table);

    for (auto &varvar : varvartable)
        inthash_add(&h_varvar, varvar.x, varvar.y, 0);

    initsoundhashnames();

    for (tokenmap_t const & keyword : vm_keywords)
        hash_add(&h_keywords, keyword.token, keyword.val, 0);
}

#undef STRUCT_HASH_SETUP

// "magic" number for { and }, overrides line number in compiled code for later detection
#define IFELSE_MAGIC 31337
static int32_t g_skipBranch;

static int32_t C_SetScriptSize(int32_t newsize)
{
    intptr_t const oscript = (intptr_t)apScript;
    intptr_t *newscript;
    intptr_t i, j;
    int32_t osize = g_scriptSize;
    char *scriptptrs;
    char *newbitptr;

    scriptptrs = (char *)Xcalloc(1, g_scriptSize * sizeof(uint8_t));

    for (i=g_scriptSize-1; i>=0; i--)
    {
        if (BITPTR_IS_POINTER(i))
        {
            if (EDUKE32_PREDICT_FALSE((intptr_t)apScript[i] < (intptr_t)&apScript[0] || (intptr_t)apScript[i] >= (intptr_t)&apScript[g_scriptSize]))
            {
                g_errorCnt++;
                buildprint("Internal compiler error at ", i, " (0x", hex(i), ")\n");
                VM_ScriptInfo(&apScript[i], 16);
            }

            scriptptrs[i] = 1;
            apScript[i] -= (intptr_t)&apScript[0];
        }
        else scriptptrs[i] = 0;
    }

    G_Util_PtrToIdx2(&g_tile[0].execPtr, MAXTILES, sizeof(tiledata_t), apScript, P2I_FWD_NON0);
    G_Util_PtrToIdx2(&g_tile[0].loadPtr, MAXTILES, sizeof(tiledata_t), apScript, P2I_FWD_NON0);

    newscript = (intptr_t *)Xrealloc(apScript, newsize * sizeof(intptr_t));
    newbitptr = (char *)Xcalloc(1,(((newsize+7)>>3)+1) * sizeof(uint8_t));

    if (newsize >= osize)
    {
        Bmemset(&newscript[0]+osize,0,(newsize-osize) * sizeof(uint8_t));
        Bmemcpy(newbitptr,bitptr,sizeof(uint8_t) *((osize+7)>>3));
    }
    else
        Bmemcpy(newbitptr,bitptr,sizeof(uint8_t) *((newsize+7)>>3));

    Xfree(bitptr);
    bitptr = newbitptr;
    if (apScript != newscript)
    {
        buildprint("Relocating compiled code from to 0x", hex((intptr_t)apScript), " to 0x", hex((intptr_t)newscript), "\n");
        apScript = newscript;
    }

    g_scriptSize = newsize;
    g_scriptPtr = apScript + (intptr_t)g_scriptPtr - oscript;

    if (g_caseScriptPtr)
        g_caseScriptPtr = apScript + (intptr_t)g_caseScriptPtr - oscript;

    for (i=(((newsize>=osize)?osize:newsize))-1; i>=0; i--)
        if (scriptptrs[i])
        {
            j = (intptr_t)apScript[i]+(intptr_t)&apScript[0];
            apScript[i] = j;
        }

    G_Util_PtrToIdx2(&g_tile[0].execPtr, MAXTILES, sizeof(tiledata_t), apScript, P2I_BACK_NON0);
    G_Util_PtrToIdx2(&g_tile[0].loadPtr, MAXTILES, sizeof(tiledata_t), apScript, P2I_BACK_NON0);

    Xfree(scriptptrs);
    return 0;
}

static inline int32_t ispecial(const char c)
{
    return (c == ' ' || c == 0x0d || c == '(' || c == ')' ||
        c == ',' || c == ';' || (c == 0x0a /*&& ++g_lineNumber*/));
}

static inline void C_NextLine(void)
{
    while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
        textptr++;
}

static inline void C_SkipSpace(void)
{
    while (*textptr == ' ' || *textptr == '\t')
        textptr++;
}

static int32_t g_gotComment = 0;

static int32_t C_SkipComments(void)
{
    g_gotComment = 0;
    do
    {
        switch (*textptr)
        {
        case '\n':
            g_lineNumber++;
            fallthrough__;
        case ' ':
        case '\t':
        case '\r':
        case 0x1a:
            textptr++;
            break;
        case '/':
            switch (textptr[1])
            {
            case '/': // C++ style comment
                if (!(g_errorCnt || g_warningCnt) && g_scriptDebug > 1)
                    initprintf("%s:%d: debug: got comment.\n",g_scriptFileName,g_lineNumber);
                C_NextLine();
                g_gotComment = 1;
                continue;
            case '*': // beginning of a C style comment
                if (!(g_errorCnt || g_warningCnt) && g_scriptDebug > 1)
                    initprintf("%s:%d: debug: got start of comment block.\n",g_scriptFileName,g_lineNumber);
                do
                {
                    if (*textptr == '\n')
                        g_lineNumber++;
                    textptr++;
                }
                while (*textptr && (textptr[0] != '*' || textptr[1] != '/'));

                if (EDUKE32_PREDICT_FALSE(!*textptr))
                {
                    if (!(g_errorCnt || g_warningCnt) && g_scriptDebug)
                        initprintf("%s:%d: debug: EOF in comment!\n",g_scriptFileName,g_lineNumber);
                    C_ReportError(-1);
                    initprintf("%s:%d: error: found `/*' with no `*/'.\n",g_scriptFileName,g_lineNumber);
                    g_parsingActorPtr = g_processingState = g_numBraces = 0;
                    g_errorCnt++;
                    continue;
                }

                if (!(g_errorCnt || g_warningCnt) && g_scriptDebug > 1)
                    initprintf("%s:%d: debug: got end of comment block.\n",g_scriptFileName,g_lineNumber);

                textptr+=2;
                g_gotComment = 1;
                continue;
            default:
                C_ReportError(-1);
                initprintf("%s:%d: error: malformed comment.\n", g_scriptFileName, g_lineNumber);
                C_NextLine();
                g_errorCnt++;
                continue;
            }
            break;

        default:
            if (ispecial(*textptr))
            {
                textptr++;
                continue;
            }
            fallthrough__;
        case 0: // EOF
            return ((g_scriptPtr-apScript) > (g_scriptSize-32)) ? C_SetScriptSize(g_scriptSize<<1) : 0;
        }
    }
    while (1);
}

static inline int GetDefID(char const *label) { return hash_find(&h_gamevars, label); }

#define LAST_LABEL (label+(g_labelCnt<<6))
static inline int32_t isaltok(const char c)
{
    return (isalnum(c) || c == '{' || c == '}' || c == '/' || c == '\\' || c == '*' || c == '-' || c == '_' ||
            c == '.');
}

static inline int32_t C_IsLabelChar(const char c, int32_t const i)
{
    return (isalnum(c) || c == '_' || c == '*' || c == '?' || (i > 0 && (c == '+' || c == '-' || c == '.')));
}

static inline int32_t C_GetLabelNameID(const memberlabel_t *pLabel, hashtable_t const * const table, const char *psz)
{
    // find the label psz in the table pLabel.
    // returns the ID for the label, or -1

    int32_t l = hash_findcase(table, psz);
    return (l >= 0) ? pLabel[l].lId : -1;
}

static inline int32_t C_GetLabelNameOffset(hashtable_t const * const table, const char *psz)
{
    // find the label psz in the table pLabel.
    // returns the offset in the array for the label, or -1

    return hash_findcase(table, psz);
}

static void C_GetNextLabelName(void)
{
    int32_t i = 0;

    C_SkipComments();

//    while (ispecial(*textptr) == 0 && *textptr!='['&& *textptr!=']' && *textptr!='\t' && *textptr!='\n' && *textptr!='\r')
    while (C_IsLabelChar(*textptr, i))
        label[(g_labelCnt<<6)+(i++)] = *(textptr++);

    label[(g_labelCnt<<6)+i] = 0;

    if (!(g_errorCnt|g_warningCnt) && g_scriptDebug > 1)
        initprintf("%s:%d: debug: label `%s'.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
}

static inline void scriptWriteValue(int32_t const value)
{
    BITPTR_CLEAR(g_scriptPtr-apScript);
    *g_scriptPtr++ = value;
}

// addresses passed to these functions must be within the block of memory pointed to by apScript
static inline void scriptWriteAtOffset(int32_t const value, intptr_t * const addr)
{
    BITPTR_CLEAR(addr-apScript);
    *(addr) = value;
}

static inline void scriptWritePointer(intptr_t const value, intptr_t * const addr)
{
    BITPTR_SET(addr-apScript);
    *(addr) = value;
}

static int32_t C_GetKeyword(void)
{
    int32_t i;
    char *temptextptr;

    C_SkipComments();

    temptextptr = textptr;

    if (*temptextptr == 0) // EOF
        return -2;

    while (isaltok(*temptextptr) == 0)
    {
        temptextptr++;
        if (*temptextptr == 0)
            return 0;
    }

    i = 0;
    while (isaltok(*temptextptr))
        tempbuf[i++] = *(temptextptr++);
    tempbuf[i] = 0;

    return hash_find(&h_keywords,tempbuf);
}

static int32_t C_GetNextKeyword(void) //Returns its code #
{
    int32_t i, l;

    C_SkipComments();

    if (*textptr == 0) // EOF
        return -2;

    l = 0;
    while (isaltok(*(textptr+l)))
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    if (EDUKE32_PREDICT_TRUE((i = hash_find(&h_keywords,tempbuf)) >= 0))
    {
        if (i == CON_LEFTBRACE || i == CON_RIGHTBRACE || i == CON_NULLOP)
            scriptWriteValue(i | (VM_IFELSE_MAGIC<<12));
        else scriptWriteValue(i | LINE_NUMBER);

        textptr += l;

        if (!(g_errorCnt || g_warningCnt) && g_scriptDebug)
            initprintf("%s:%d: debug: keyword `%s'.\n", g_scriptFileName, g_lineNumber, tempbuf);
        return i;
    }

    textptr += l;
    g_errorCnt++;

    if (EDUKE32_PREDICT_FALSE((tempbuf[0] == '{' || tempbuf[0] == '}') && tempbuf[1] != 0))
    {
        C_ReportError(-1);
        initprintf("%s:%d: error: expected whitespace between `%c' and `%s'.\n",g_scriptFileName,g_lineNumber,tempbuf[0],tempbuf+1);
    }
    else C_ReportError(ERROR_EXPECTEDKEYWORD);

    return -1;
}

static int32_t parse_decimal_number(void)  // (textptr)
{
    // decimal constants -- this is finicky business
    int64_t num = strtoll(textptr, NULL, 10);  // assume long long to be int64_t

    if (EDUKE32_PREDICT_TRUE(num >= INT32_MIN && num <= INT32_MAX))
    {
        // all OK
    }
    else if (EDUKE32_PREDICT_FALSE(num > INT32_MAX && num <= UINT32_MAX))
    {
        // Number interpreted as uint32, but packed as int32 (on 32-bit archs)
        // (CON code in the wild exists that does this).  Note that such conversion
        // is implementation-defined (C99 6.3.1.3) but GCC does the 'expected' thing.
#if 0
        initprintf("%s:%d: warning: number greater than INT32_MAX converted to a negative one.\n",
                   g_szScriptFileName,g_lineNumber);
        g_numCompilerWarnings++;
#endif
    }
    else
    {
        // out of range, this is arguably worse

        initprintf("%s:%d: warning: number out of the range of a 32-bit integer encountered.\n",
                   g_scriptFileName,g_lineNumber);
        g_warningCnt++;
    }

    return (int32_t)num;
}

static int32_t parse_hex_constant(const char *hexnum)
{
    uint64_t x;
    sscanf(hexnum, "%" PRIx64 "", &x);

    if (EDUKE32_PREDICT_FALSE(x > UINT32_MAX))
    {
        initprintf(g_scriptFileName, ":", g_lineNumber, ": warning: number 0x", hex(x), " truncated to 32 bits.\n");
        g_warningCnt++;
    }

    return x;
}

static void C_GetNextVarType(int32_t type)
{
    int32_t id    = 0;
    int32_t flags = 0;

    auto varptr = g_scriptPtr;

    C_SkipComments();

    if (!type && !g_labelsOnly && (isdigit(*textptr) || ((*textptr == '-') && (isdigit(*(textptr+1))))))
    {
        scriptWriteValue(GV_FLAG_CONSTANT);

        if (tolower(textptr[1])=='x')  // hex constants
            scriptWriteValue(parse_hex_constant(textptr+2));
        else
            scriptWriteValue(parse_decimal_number());

        if (!(g_errorCnt || g_warningCnt) && g_scriptDebug)
            initprintf("%s:%d: debug: constant %ld in place of gamevar.\n", g_scriptFileName, g_lineNumber, (long)(g_scriptPtr[-1]));
#if 1
        while (!ispecial(*textptr) && *textptr != ']') textptr++;
#else
        C_GetNextLabelName();
#endif
        return;
    }
    else if (*textptr == '-'/* && !isdigit(*(textptr+1))*/)
    {
        if (EDUKE32_PREDICT_FALSE(type))
        {
            g_errorCnt++;
            C_ReportError(ERROR_SYNTAXERROR);
            C_GetNextLabelName();
            return;
        }

        if (!(g_errorCnt || g_warningCnt) && g_scriptDebug)
            initprintf("%s:%d: debug: flagging gamevar as negative.\n", g_scriptFileName, g_lineNumber); //,Batol(textptr));

        flags = GV_FLAG_NEGATIVE;
        textptr++;
    }

    C_GetNextLabelName();

    if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,LAST_LABEL)>=0))
    {
        g_errorCnt++;
        C_ReportError(ERROR_ISAKEYWORD);
        return;
    }

    C_SkipComments();

    id=GetDefID(LAST_LABEL);
    if (id<0)   //gamevar not found
    {
        if (EDUKE32_PREDICT_TRUE(!type && !g_labelsOnly))
        {
            //try looking for a define instead
            Bstrcpy(tempbuf,LAST_LABEL);
            id = hash_find(&h_labels,tempbuf);

            if (EDUKE32_PREDICT_TRUE(id>=0 && labeltype[id] & LABEL_DEFINE))
            {
                if (!(g_errorCnt || g_warningCnt) && g_scriptDebug)
                    initprintf("%s:%d: debug: label `%s' in place of gamevar.\n",g_scriptFileName,g_lineNumber,label+(id<<6));

                scriptWriteValue(GV_FLAG_CONSTANT);
                scriptWriteValue(labelcode[id]);
                return;
            }
        }

        g_errorCnt++;
        C_ReportError(ERROR_NOTAGAMEVAR);
        return;
    }

    if (EDUKE32_PREDICT_FALSE(type == GAMEVAR_READONLY && aGameVars[id].flags & GAMEVAR_READONLY))
    {
        g_errorCnt++;
        C_ReportError(ERROR_VARREADONLY);
        return;
    }
    else if (EDUKE32_PREDICT_FALSE(aGameVars[id].flags & type))
    {
        g_errorCnt++;
        C_ReportError(ERROR_VARTYPEMISMATCH);
        return;
    }

    if (g_scriptDebug > 1 && !g_errorCnt && !g_warningCnt)
        initprintf("%s:%d: debug: gamevar `%s'.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);

    scriptWriteValue(id|flags);
}

#define C_GetNextVar() C_GetNextVarType(0)

static FORCE_INLINE void C_GetManyVarsType(int32_t type, int num)
{
    for (; num>0; --num)
        C_GetNextVarType(type);
}

#define C_GetManyVars(num) C_GetManyVarsType(0,num)

// returns:
//  -1 on EOF or wrong type or error
//   0 if literal value
//   LABEL_* (>0) if that type and matched
//
// *g_scriptPtr will contain the value OR 0 if wrong type or error
static int32_t C_GetNextValue(int32_t type)
{
    C_SkipComments();

    if (*textptr == 0) // EOF
        return -1;

    int32_t l = 0;

    while (isaltok(*(textptr+l)))
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,tempbuf /*label+(g_numLabels<<6)*/)>=0))
    {
        g_errorCnt++;
        C_ReportError(ERROR_ISAKEYWORD);
        textptr+=l;
    }

    int32_t i = hash_find(&h_labels,tempbuf);

    if (i>=0)
    {
        if (EDUKE32_PREDICT_TRUE(labeltype[i] & type))
        {
            if (!(g_errorCnt || g_warningCnt) && g_scriptDebug > 1)
            {
                char *gl = C_GetLabelType(labeltype[i]);
                initprintf("%s:%d: debug: %s label `%s'.\n",g_scriptFileName,g_lineNumber,gl,label+(i<<6));
                Xfree(gl);
            }

            BITPTR_CLEAR(g_scriptPtr-apScript);
            *(g_scriptPtr++) = labelcode[i];

            textptr += l;
            return labeltype[i];
        }

        BITPTR_CLEAR(g_scriptPtr-apScript);
        *(g_scriptPtr++) = 0;
        textptr += l;
        char *el = C_GetLabelType(type);
        char *gl = C_GetLabelType(labeltype[i]);
        C_ReportError(-1);
        initprintf("%s:%d: warning: expected %s, found %s.\n",g_scriptFileName,g_lineNumber,el,gl);
        g_warningCnt++;
        Xfree(el);
        Xfree(gl);
        return -1;  // valid label name, but wrong type
    }

    if (EDUKE32_PREDICT_FALSE(isdigit(*textptr) == 0 && *textptr != '-'))
    {
        C_ReportError(ERROR_PARAMUNDEFINED);
        g_errorCnt++;
        BITPTR_CLEAR(g_scriptPtr-apScript);
        *g_scriptPtr = 0;
        g_scriptPtr++;
        textptr+=l;
        if (!l) textptr++;
        return -1; // error!
    }

    if (EDUKE32_PREDICT_FALSE(isdigit(*textptr) && g_labelsOnly))
    {
        C_ReportError(WARNING_LABELSONLY);
        g_warningCnt++;
    }

    i = l-1;
    do
    {
        // FIXME: check for 0-9 A-F for hex
        if (textptr[0] == '0' && textptr[1] == 'x') break; // kill the warning for hex
        if (EDUKE32_PREDICT_FALSE(!isdigit(textptr[i--])))
        {
            C_ReportError(-1);
            initprintf("%s:%d: warning: invalid character `%c' in definition!\n",g_scriptFileName,g_lineNumber,textptr[i+1]);
            g_warningCnt++;
            break;
        }
    }
    while (i > 0);

    BITPTR_CLEAR(g_scriptPtr-apScript);

    if (textptr[0] == '0' && tolower(textptr[1])=='x')
        *g_scriptPtr = parse_hex_constant(textptr+2);
    else
        *g_scriptPtr = parse_decimal_number();

    if (!(g_errorCnt || g_warningCnt) && g_scriptDebug > 1)
        initprintf("%s:%d: debug: constant %ld.\n",
                   g_scriptFileName,g_lineNumber,(long)*g_scriptPtr);

    g_scriptPtr++;

    textptr += l;

    return 0;   // literal value
}

static int32_t C_CheckMalformedBranch(intptr_t lastScriptPtr)
{
    switch (C_GetKeyword())
    {
    case CON_RIGHTBRACE:
    case CON_ENDA:
    case CON_ENDS:
    case CON_ELSE:
        g_scriptPtr = lastScriptPtr + &apScript[0];
        g_skipBranch = 1;
        C_ReportError(-1);
        g_warningCnt++;
        initprintf("%s:%d: warning: malformed `%s' branch\n",g_scriptFileName,g_lineNumber,
                   VM_GetKeywordForID(*(g_scriptPtr) & VM_INSTMASK));
        return 1;
    }
    return 0;
}

static int32_t C_CheckEmptyBranch(int32_t tw, intptr_t lastScriptPtr)
{
    // ifrnd and the others actually do something when the condition is executed
    if ((Bstrncmp(VM_GetKeywordForID(tw), "if", 2) && tw != CON_ELSE) ||
            tw == CON_IFRND || tw == CON_IFHITWEAPON || tw == CON_IFCANSEE || tw == CON_IFCANSEETARGET ||
            tw == CON_IFPDISTL || tw == CON_IFPDISTG || tw == CON_IFGOTWEAPONCE)
    {
        g_skipBranch = 0;
        return 0;
    }

    if ((*(g_scriptPtr) & VM_INSTMASK) != CON_NULLOP || *(g_scriptPtr)>>12 != IFELSE_MAGIC)
        g_skipBranch = 0;

    if (EDUKE32_PREDICT_FALSE(g_skipBranch))
    {
        C_ReportError(-1);
        g_warningCnt++;
        g_scriptPtr = lastScriptPtr + &apScript[0];
        initprintf("%s:%d: warning: empty `%s' branch\n",g_scriptFileName,g_lineNumber,
                   VM_GetKeywordForID(*(g_scriptPtr) & VM_INSTMASK));
        *(g_scriptPtr) = (CON_NULLOP + (IFELSE_MAGIC<<12));
        return 1;
    }
    return 0;
}

static void C_Include(const char *confile)
{
	auto fp = fileSystem.OpenFileReader(confile,0);

    if (!fp.isOpen())
    {
        g_errorCnt++;
        initprintf("%s:%d: error: could not find file `%s'.\n",g_scriptFileName,g_lineNumber,confile);
        return;
    }

	int32_t j = fp.GetLength();

    char *mptr = (char *)Xmalloc(j+1);

    initprintf("Including: %s (%d bytes)\n",confile, j);

    fp.Read(mptr, j);
    fp.Close();
    g_scriptcrc = Bcrc32(mptr, j, g_scriptcrc);
    mptr[j] = 0;

    if (*textptr == '"') // skip past the closing quote if it's there so we don't screw up the next line
        textptr++;

    char *origtptr = textptr;
    char parentScriptFileName[255];

    Bstrcpy(parentScriptFileName, g_scriptFileName);
    Bstrcpy(g_scriptFileName, confile);

    int32_t temp_ScriptLineNumber = g_lineNumber;
    g_lineNumber = 1;

    int32_t temp_ifelse_check = g_checkingIfElse;
    g_checkingIfElse = 0;

    textptr = mptr;

    C_SkipComments();
    C_ParseCommand(1);

    Bstrcpy(g_scriptFileName, parentScriptFileName);

    g_totalLines += g_lineNumber;
    g_lineNumber = temp_ScriptLineNumber;
    g_checkingIfElse = temp_ifelse_check;

    textptr = origtptr;

    Xfree(mptr);
}

void G_DoGameStartup(const int32_t *params)
{
    int j = 0;

    ud.const_visibility               = params[j++];
    g_impactDamage                    = params[j++];
    g_player[0].ps->max_shield_amount = params[j++];
    g_player[0].ps->max_player_health = g_player[0].ps->max_shield_amount;
    g_maxPlayerHealth                 = g_player[0].ps->max_player_health;
    g_startArmorAmount                = params[j++];
    g_actorRespawnTime                = params[j++];
    g_itemRespawnTime                 = params[j++];
    g_playerFriction                  = params[j++];
    g_spriteGravity                   = params[j++];
    g_rpgRadius                       = params[j++];
    g_pipebombRadius                  = params[j++];
    g_shrinkerRadius                  = params[j++];
    g_tripbombRadius                  = params[j++];
    g_morterRadius                    = params[j++];
    g_bouncemineRadius                = params[j++];
    g_seenineRadius                   = params[j++];

    g_player[0].ps->max_ammo_amount[1] = params[j++];
    g_player[0].ps->max_ammo_amount[2] = params[j++];
    g_player[0].ps->max_ammo_amount[3] = params[j++];
    g_player[0].ps->max_ammo_amount[4] = params[j++];
    g_player[0].ps->max_ammo_amount[5] = params[j++];
    g_player[0].ps->max_ammo_amount[6] = params[j++];
    g_player[0].ps->max_ammo_amount[7] = params[j++];
    g_player[0].ps->max_ammo_amount[8] = params[j++];
    g_player[0].ps->max_ammo_amount[9] = params[j++];
    g_player[0].ps->max_ammo_amount[11] = params[j++];

    if (RR)
        g_player[0].ps->max_ammo_amount[12] = params[j++];

    g_damageCameras     = params[j++];
    g_numFreezeBounces  = params[j++];
    g_freezerSelfDamage = params[j++];
    g_deleteQueueSize   = clamp(params[j++], 0, 1024);
    g_tripbombLaserMode = params[j++];

    if (RRRA)
    {
        g_player[0].ps->max_ammo_amount[13] = params[j++];
        g_player[0].ps->max_ammo_amount[14] = params[j++];
        g_player[0].ps->max_ammo_amount[16] = params[j++];
    }
}

void C_DefineMusic(int volumeNum, int levelNum, const char *fileName)
{
    Bassert((unsigned)volumeNum < MAXVOLUMES+1);
    Bassert((unsigned)levelNum < MAXLEVELS);

    mapList[(MAXLEVELS*volumeNum)+levelNum].music = fileName;
}

void C_DefineVolumeFlags(int32_t vol, int32_t flags)
{
    Bassert((unsigned)vol < MAXVOLUMES);

    gVolumeFlags[vol] = flags;
}

int32_t C_AllocQuote(int32_t qnum)
{
    Bassert((unsigned)qnum < MAXQUOTES);
    return 1;
}

void C_InitQuotes(void)
{
#if 0	// if we want to keep this it must be done differently. This does not play nice with text substitution.
	auto openkeys = Bindings.GetKeysForCommand("+open");
	if (openkeys.Size())
	{
		auto OpenGameFunc = C_NameKeys(openkeys.Data(), 1);
		quoteMgr.Substitute(QUOTE_DEAD, "SPACE", OpenGameFunc);
		quoteMgr.Substitute(QUOTE_DEAD, "OPEN", OpenGameFunc);
		quoteMgr.Substitute(QUOTE_DEAD, "USE", OpenGameFunc);
	}
#endif

    g_numObituaries = 48;
    for (bssize_t i = g_numObituaries - 1; i >= 0; i--)
    {
		quoteMgr.FormatQuote(i + OBITQUOTEINDEX, "$TXT_OBITUARY%d", i + 1);
    }

    g_numSelfObituaries = 6;
    for (bssize_t i = g_numSelfObituaries - 1; i >= 0; i--)
    {
		quoteMgr.FormatQuote(i + SUICIDEQUOTEINDEX, "$TXT_SELFOBIT%d", i + 1);
    }
}

static inline void C_BitOrNextValue(int32_t *valptr)
{
    C_GetNextValue(LABEL_DEFINE);
    g_scriptPtr--;
    *valptr |= *g_scriptPtr;
}

static inline void C_FinishBitOr(int32_t value)
{
    BITPTR_CLEAR(g_scriptPtr-apScript);
    *g_scriptPtr++ = value;
}

static void scriptUpdateOpcodeForVariableType(intptr_t *ins)
{
    int opcode = -1;

    if (opcode != -1)
    {
        if (g_scriptDebug > 1 && !g_errorCnt && !g_warningCnt)
        {
            initprintf("%s:%d: %s -> %s for var %s\n", g_scriptFileName, g_lineNumber,
                        VM_GetKeywordForID(*ins & VM_INSTMASK), VM_GetKeywordForID(opcode), aGameVars[ins[1] & (MAXGAMEVARS-1)].szLabel);
        }

        scriptWriteAtOffset(opcode | LINE_NUMBER, ins);
    }
}

static int32_t C_ParseCommand(int32_t loop)
{
    int32_t i, j=0, k=0, tw;
    TArray<char> buffer;

    do
    {
        if (EDUKE32_PREDICT_FALSE(g_errorCnt > 63 || (*textptr == '\0') || (*(textptr+1) == '\0') || C_SkipComments()))
            return 1;

        if (EDUKE32_PREDICT_FALSE(g_scriptDebug))
            C_ReportError(-1);

        switch ((g_lastKeyword = tw = C_GetNextKeyword()))
        {
        default:
        case -1:
        case -2:
            return 1; //End
        case CON_STATE:
            if (!g_parsingActorPtr && g_processingState == 0)
            {
                C_GetNextLabelName();
                g_scriptPtr--;
                labelcode[g_labelCnt] = g_scriptPtr-apScript;
                labeltype[g_labelCnt] = LABEL_STATE;

                g_processingState = 1;
                Bsprintf(g_szCurrentBlockName,"%s",label+(g_labelCnt<<6));

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,label+(g_labelCnt<<6))>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                hash_add(&h_labels,label+(g_labelCnt<<6),g_labelCnt,0);
                g_labelCnt++;
                continue;
            }

            C_GetNextLabelName();

            if (EDUKE32_PREDICT_FALSE((j = hash_find(&h_labels,label+(g_labelCnt<<6))) < 0))
            {
                C_ReportError(-1);
                initprintf("%s:%d: error: state `%s' not found.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
                g_errorCnt++;
                g_scriptPtr++;
                continue;
            }

            if (EDUKE32_PREDICT_FALSE((labeltype[j] & LABEL_STATE) != LABEL_STATE))
            {
                char *gl = (char *) C_GetLabelType(labeltype[j]);
                C_ReportError(-1);
                initprintf("%s:%d: warning: expected state, found %s.\n", g_scriptFileName, g_lineNumber, gl);
                g_warningCnt++;
                Xfree(gl);
                *(g_scriptPtr-1) = CON_NULLOP; // get rid of the state, leaving a nullop to satisfy if conditions
                BITPTR_CLEAR(g_scriptPtr-apScript-1);
                continue;  // valid label name, but wrong type
            }

            if (!(g_errorCnt || g_warningCnt) && g_scriptDebug > 1)
                initprintf("%s:%d: debug: state label `%s'.\n", g_scriptFileName, g_lineNumber, label+(j<<6));
            *g_scriptPtr = (intptr_t) (apScript+labelcode[j]);

            // 'state' type labels are always script addresses, as far as I can see
            BITPTR_SET(g_scriptPtr-apScript);

            g_scriptPtr++;
            continue;

        case CON_ENDS:
            if (EDUKE32_PREDICT_FALSE(g_processingState == 0))
            {
                C_ReportError(-1);
                initprintf("%s:%d: error: found `ends' without open `state'.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
            }
            //            else
            {
                if (EDUKE32_PREDICT_FALSE(g_numBraces > 0))
                {
                    C_ReportError(ERROR_OPENBRACKET);
                    g_errorCnt++;
                }
                else if (EDUKE32_PREDICT_FALSE(g_numBraces < 0))
                {
                    C_ReportError(ERROR_CLOSEBRACKET);
                    g_errorCnt++;
                }

                g_processingState = 0;
                Bsprintf(g_szCurrentBlockName,"(none)");
            }
            continue;

        case CON_GAMEVAR:
        {
            // syntax: gamevar <var1> <initial value> <flags>
            // defines var1 and sets initial value.
            // flags are used to define usage
            // (see top of this files for flags)

            if (EDUKE32_PREDICT_FALSE(isdigit(*textptr) || (*textptr == '-')))
            {
                g_errorCnt++;
                C_ReportError(ERROR_SYNTAXERROR);
                C_NextLine();
                continue;
            }

            g_scriptPtr--;

            C_GetNextLabelName();

            if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords, LAST_LABEL)>=0))
            {
                g_warningCnt++;
                C_ReportError(WARNING_VARMASKSKEYWORD);
                hash_delete(&h_keywords, LAST_LABEL);
            }

            int32_t defaultValue = 0;
            int32_t varFlags     = 0;

            if (C_GetKeyword() == -1)
            {
                C_GetNextValue(LABEL_DEFINE); // get initial value
                defaultValue = *(--g_scriptPtr);

                j = 0;

                while (C_GetKeyword() == -1)
                    C_BitOrNextValue(&j);

                C_FinishBitOr(j);
                varFlags = *(--g_scriptPtr);

                if (EDUKE32_PREDICT_FALSE((*(g_scriptPtr)&GAMEVAR_USER_MASK)==(GAMEVAR_PERPLAYER|GAMEVAR_PERACTOR)))
                {
                    g_warningCnt++;
                    varFlags ^= GAMEVAR_PERPLAYER;
                    C_ReportError(WARNING_BADGAMEVAR);
                }
            }

            Gv_NewVar(LAST_LABEL, defaultValue, varFlags);
            continue;
        }

        case CON_DEFINE:
            {
                C_GetNextLabelName();

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,label+(g_labelCnt<<6))>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                C_GetNextValue(LABEL_DEFINE);

                i = hash_find(&h_labels,label+(g_labelCnt<<6));
                if (i>=0)
                {
                    // if (i >= g_numDefaultLabels)

                    if (EDUKE32_PREDICT_FALSE(labelcode[i] != *(g_scriptPtr-1)))
                    {
                        g_warningCnt++;
                        initprintf("%s:%d: warning: ignored redefinition of `%s' to %d (old: %d).\n",g_scriptFileName,
                                   g_lineNumber,label+(g_labelCnt<<6), (int32_t)(*(g_scriptPtr-1)), labelcode[i]);
                    }
                }
                else
                {
                    hash_add(&h_labels,label+(g_labelCnt<<6),g_labelCnt,0);
                    labeltype[g_labelCnt] = LABEL_DEFINE;
                    labelcode[g_labelCnt++] = *(g_scriptPtr-1);
                }
                g_scriptPtr -= 2;
                continue;
            }

        case CON_PALFROM:
            for (j=3; j>=0; j--)
            {
                if (C_GetKeyword() == -1)
                    C_GetNextValue(LABEL_DEFINE);
                else break;
            }

            while (j>-1)
            {
                BITPTR_CLEAR(g_scriptPtr-apScript);
                *g_scriptPtr++ = 0;
                j--;
            }
            continue;

        case CON_MOVE:
            if (g_parsingActorPtr || g_processingState)
            {
                if (EDUKE32_PREDICT_FALSE((C_GetNextValue(LABEL_MOVE|LABEL_DEFINE) == 0) && (*(g_scriptPtr-1) != 0) && (*(g_scriptPtr-1) != 1)))
                {
                    C_ReportError(-1);
                    BITPTR_CLEAR(g_scriptPtr-apScript-1);
                    *(g_scriptPtr-1) = 0;
                    initprintf("%s:%d: warning: expected a move, found a constant.\n",g_scriptFileName,g_lineNumber);
                    g_warningCnt++;
                }

                j = 0;
                while (C_GetKeyword() == -1)
                    C_BitOrNextValue(&j);

                C_FinishBitOr(j);
            }
            else
            {
                g_scriptPtr--;
                C_GetNextLabelName();
                // Check to see it's already defined

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,label+(g_labelCnt<<6))>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE((i = hash_find(&h_labels,label+(g_labelCnt<<6))) >= 0))
                {
                    g_warningCnt++;
                    initprintf("%s:%d: warning: duplicate move `%s' ignored.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
                }
                else
                {
                    hash_add(&h_labels,label+(g_labelCnt<<6),g_labelCnt,0);
                    labeltype[g_labelCnt] = LABEL_MOVE;
                    labelcode[g_labelCnt++] = g_scriptPtr-apScript;
                }

                for (j=1; j>=0; j--)
                {
                    if (C_GetKeyword() != -1) break;
                    C_GetNextValue(LABEL_DEFINE);
                }

                for (k=j; k>=0; k--)
                {
                    BITPTR_CLEAR(g_scriptPtr-apScript);
                    *g_scriptPtr = 0;
                    g_scriptPtr++;
                }
            }
            continue;

        case CON_MUSIC:
            {
                // NOTE: this doesn't get stored in the PCode...

                // music 1 stalker.mid dethtoll.mid streets.mid watrwld1.mid snake1.mid
                //    thecall.mid ahgeez.mid dethtoll.mid streets.mid watrwld1.mid snake1.mid
                g_scriptPtr--;
                C_GetNextValue(LABEL_DEFINE); // Volume Number (0/4)
                g_scriptPtr--;

                k = *g_scriptPtr-1;  // 0-based volume number. -1 or MAXVOLUMES: "special"
                if (k == -1)
                    k = MAXVOLUMES;

                if (EDUKE32_PREDICT_FALSE((unsigned)k >= MAXVOLUMES+1)) // if it's not background or special music
                {
                    g_errorCnt++;
                    C_ReportError(-1);
                    initprintf("%s:%d: error: volume number must be between 0 and MAXVOLUMES+1=%d.\n",
                               g_scriptFileName, g_lineNumber, MAXVOLUMES+1);
                    continue;

                }

                i = 0;
                // get the file name...
                while (C_GetKeyword() == -1)
                {
                    C_SkipComments();

                    j = 0;
                    tempbuf[j] = '/';
                    while (isaltok(*(textptr+j)))
                    {
                        tempbuf[j+1] = textptr[j];
                        j++;
                    }
                    tempbuf[j+1] = '\0';

                    C_DefineMusic(k, i, tempbuf);

                    textptr += j;

                    if (i >= MAXLEVELS)
                        break;
                    i++;
                }
            }
            continue;

        case CON_INCLUDE:
            g_scriptPtr--;

            C_SkipComments();
            while (isaltok(*textptr) == 0)
            {
                textptr++;
                if (*textptr == 0) break;
            }

            j = 0;
            while (isaltok(*textptr))
            {
                tempbuf[j] = *(textptr++);
                j++;
            }
            tempbuf[j] = '\0';

            C_Include(tempbuf);
            continue;

        case CON_AI:
            if (g_parsingActorPtr || g_processingState)
            {
                C_GetNextValue(LABEL_AI);
            }
            else
            {
                g_scriptPtr--;
                C_GetNextLabelName();

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,label+(g_labelCnt<<6))>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                i = hash_find(&h_labels,label+(g_labelCnt<<6));
                if (EDUKE32_PREDICT_FALSE(i>=0))
                {
                    g_warningCnt++;
                    initprintf("%s:%d: warning: duplicate ai `%s' ignored.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
                }
                else
                {
                    labeltype[g_labelCnt] = LABEL_AI;
                    hash_add(&h_labels,label+(g_labelCnt<<6),g_labelCnt,0);
                    labelcode[g_labelCnt++] = g_scriptPtr-apScript;
                }

                for (j=0; j<3; j++)
                {
                    if (C_GetKeyword() != -1) break;
                    if (j == 1)
                        C_GetNextValue(LABEL_ACTION);
                    else if (j == 2)
                    {
                        if (EDUKE32_PREDICT_FALSE((C_GetNextValue(LABEL_MOVE|LABEL_DEFINE) == 0) &&
                            (*(g_scriptPtr-1) != 0) && (*(g_scriptPtr-1) != 1)))
                        {
                            C_ReportError(-1);
                            BITPTR_CLEAR(g_scriptPtr-apScript-1);
                            *(g_scriptPtr-1) = 0;
                            initprintf("%s:%d: warning: expected a move, found a constant.\n",g_scriptFileName,g_lineNumber);
                            g_warningCnt++;
                        }

                        k = 0;
                        while (C_GetKeyword() == -1)
                            C_BitOrNextValue(&k);

                        C_FinishBitOr(k);
                        j = 666;
                        break;
                    }
                }

                if (j == 666)
                    continue;

                for (k=j; k<3; k++)
                {
                    BITPTR_CLEAR(g_scriptPtr-apScript);
                    *g_scriptPtr = 0;
                    g_scriptPtr++;
                }
            }
            continue;

        case CON_ACTION:
            if (g_parsingActorPtr || g_processingState)
            {
                C_GetNextValue(LABEL_ACTION);
            }
            else
            {
                g_scriptPtr--;
                C_GetNextLabelName();
                // Check to see it's already defined

                if (EDUKE32_PREDICT_FALSE(hash_find(&h_keywords,label+(g_labelCnt<<6))>=0))
                {
                    g_errorCnt++;
                    C_ReportError(ERROR_ISAKEYWORD);
                    continue;
                }

                i = hash_find(&h_labels,label+(g_labelCnt<<6));
                if (EDUKE32_PREDICT_FALSE(i>=0))
                {
                    g_warningCnt++;
                    initprintf("%s:%d: warning: duplicate action `%s' ignored.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
                }
                else
                {
                    labeltype[g_labelCnt] = LABEL_ACTION;
                    labelcode[g_labelCnt] = g_scriptPtr-apScript;
                    hash_add(&h_labels,label+(g_labelCnt<<6),g_labelCnt,0);
                    g_labelCnt++;
                }

                for (j=ACTION_PARAM_COUNT-1; j>=0; j--)
                {
                    if (C_GetKeyword() != -1) break;
                    C_GetNextValue(LABEL_DEFINE);
                }
                for (k=j; k>=0; k--)
                {
                    BITPTR_CLEAR(g_scriptPtr-apScript);
                    *(g_scriptPtr++) = 0;
                }
            }
            continue;

        case CON_ACTOR:
        case CON_USERACTOR:
            if (EDUKE32_PREDICT_FALSE(g_processingState || g_parsingActorPtr))
            {
                C_ReportError(ERROR_FOUNDWITHIN);
                g_errorCnt++;
            }

            g_numBraces = 0;
            g_scriptPtr--;
            g_parsingActorPtr = g_scriptPtr - apScript;

            if (tw == CON_USERACTOR)
            {
                C_GetNextValue(LABEL_DEFINE);
                g_scriptPtr--;
            }

            // save the actor name w/o consuming it
            C_SkipComments();
            j = 0;
            while (isaltok(*(textptr+j)))
            {
                g_szCurrentBlockName[j] = textptr[j];
                j++;
            }
            g_szCurrentBlockName[j] = 0;

            j = hash_find(&h_labels, g_szCurrentBlockName);

            if (j != -1)
                labeltype[j] |= LABEL_ACTOR;

            if (tw == CON_USERACTOR)
            {
                j = *g_scriptPtr;

                if (EDUKE32_PREDICT_FALSE(j >= 3))
                {
                    C_ReportError(-1);
                    initprintf("%s:%d: warning: invalid useractor type. Must be 0, 1, 2"
                               " (notenemy, enemy, enemystayput).\n",
                               g_scriptFileName,g_lineNumber);
                    g_warningCnt++;
                    j = 0;
                }
            }

            C_GetNextValue(LABEL_ACTOR);
            g_scriptPtr--;

            if (EDUKE32_PREDICT_FALSE((unsigned)*g_scriptPtr >= MAXTILES))
            {
                C_ReportError(ERROR_EXCEEDSMAXTILES);
                g_errorCnt++;
                continue;
            }

            g_tile[*g_scriptPtr].execPtr = apScript + g_parsingActorPtr;

            if (tw == CON_USERACTOR)
            {
                if (j & 1)
                    g_tile[*g_scriptPtr].flags |= SFLAG_BADGUY;

                if (j & 2)
                    g_tile[*g_scriptPtr].flags |= (SFLAG_BADGUY|SFLAG_BADGUYSTAYPUT);
            }

            for (j=0; j<4; j++)
            {
                BITPTR_CLEAR(g_parsingActorPtr+j);
                *((apScript+j)+g_parsingActorPtr) = 0;
                if (j == 3)
                {
                    j = 0;
                    while (C_GetKeyword() == -1)
                        C_BitOrNextValue(&j);

                    C_FinishBitOr(j);
                    break;
                }
                else
                {
                    if (C_GetKeyword() != -1)
                    {
                        for (i=4-j; i; i--)
                        {
                            BITPTR_CLEAR(g_scriptPtr-apScript);
                            *(g_scriptPtr++) = 0;
                        }
                        break;
                    }
                    switch (j)
                    {
                    case 0:
                        C_GetNextValue(LABEL_DEFINE);
                        break;
                    case 1:
                        C_GetNextValue(LABEL_ACTION);
                        break;
                    case 2:
                        // XXX: LABEL_MOVE|LABEL_DEFINE, what is this shit? compatibility?
                        // yep, it sure is :(
                        if (EDUKE32_PREDICT_FALSE((C_GetNextValue(LABEL_MOVE|LABEL_DEFINE) == 0) && (*(g_scriptPtr-1) != 0) && (*(g_scriptPtr-1) != 1)))
                        {
                            C_ReportError(-1);
                            BITPTR_CLEAR(g_scriptPtr-apScript-1);
                            *(g_scriptPtr-1) = 0;
                            initprintf("%s:%d: warning: expected a move, found a constant.\n",g_scriptFileName,g_lineNumber);
                            g_warningCnt++;
                        }
                        break;
                    }
                    if (*(g_scriptPtr-1) >= (intptr_t)&apScript[0] && *(g_scriptPtr-1) < (intptr_t)&apScript[g_scriptSize])
                        BITPTR_SET(g_parsingActorPtr+j);
                    else BITPTR_CLEAR(g_parsingActorPtr+j);
                    *((apScript+j)+g_parsingActorPtr) = *(g_scriptPtr-1);
                }
            }
            g_checkingIfElse = 0;
            continue;

        case CON_ONEVENT:
            if (EDUKE32_PREDICT_FALSE(g_processingState || g_parsingActorPtr))
            {
                C_ReportError(ERROR_FOUNDWITHIN);
                g_errorCnt++;
            }

            g_numBraces = 0;
            g_scriptPtr--;
            g_scriptEventOffset = g_parsingActorPtr = g_scriptPtr - apScript;

            C_SkipComments();
            j = 0;
            while (isaltok(*(textptr+j)))
            {
                g_szCurrentBlockName[j] = textptr[j];
                j++;
            }
            g_szCurrentBlockName[j] = 0;
            //        g_labelsOnly = 1;
            C_GetNextValue(LABEL_DEFINE);
            g_labelsOnly = 0;
            g_scriptPtr--;
            j= *g_scriptPtr;  // type of event
            g_currentEvent = j;
            //Bsprintf(g_szBuf,"Adding Event for %d at %lX",j, g_parsingEventPtr);
            //AddLog(g_szBuf);
            if (EDUKE32_PREDICT_FALSE((unsigned)j > MAXEVENTS_RR-1))
            {
                initprintf("%s:%d: error: invalid event ID.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                continue;
            }
            // if event has already been declared then store previous script location
            apScriptEvents[j] = g_scriptEventOffset;

            g_checkingIfElse = 0;

            continue;

        case CON_CSTAT:
            C_GetNextValue(LABEL_DEFINE);

            if (EDUKE32_PREDICT_FALSE(*(g_scriptPtr-1) == 32767))
            {
                *(g_scriptPtr-1) = 32768;
                C_ReportError(-1);
                initprintf("%s:%d: warning: tried to set cstat 32767, using 32768 instead.\n",g_scriptFileName,g_lineNumber);
                g_warningCnt++;
            }
            else if (EDUKE32_PREDICT_FALSE((*(g_scriptPtr-1) & 48) == 48))
            {
                i = *(g_scriptPtr-1);
                *(g_scriptPtr-1) ^= 48;
                C_ReportError(-1);
                initprintf("%s:%d: warning: tried to set cstat %d, using %d instead.\n",g_scriptFileName,g_lineNumber,i,(int32_t)(*(g_scriptPtr-1)));
                g_warningCnt++;
            }
            continue;

        case CON_HITRADIUS:
            C_GetNextValue(LABEL_DEFINE);
            C_GetNextValue(LABEL_DEFINE);
            C_GetNextValue(LABEL_DEFINE);
            fallthrough__;
        case CON_ADDAMMO:
        case CON_ADDWEAPON:
        case CON_SIZETO:
        case CON_SIZEAT:
        case CON_DEBRIS:
        case CON_ADDINVENTORY:
        case CON_GUTS:
            C_GetNextValue(LABEL_DEFINE);
            fallthrough__;
        case CON_STRENGTH:
        case CON_SHOOT:
        case CON_ADDPHEALTH:
        case CON_SPAWN:
        case CON_COUNT:
        case CON_ENDOFGAME:
        case CON_SPRITEPAL:
        case CON_CACTOR:
        case CON_MONEY:
        case CON_ADDKILLS:
        case CON_DEBUG:
        case CON_ADDSTRENGTH:
        case CON_CSTATOR:
        case CON_MAIL:
        case CON_PAPER:
        case CON_SLEEPTIME:
        case CON_CLIPDIST:
        case CON_ISDRUNK:
        case CON_ISEAT:
        case CON_NEWPIC:
        case CON_LOTSOFGLASS:
        case CON_QUOTE:
        case CON_SOUND:
        case CON_GLOBALSOUND:
        case CON_SOUNDONCE:
        case CON_STOPSOUND:
            C_GetNextValue(LABEL_DEFINE);
            continue;

        case CON_ELSE:
            {
                if (EDUKE32_PREDICT_FALSE(!g_checkingIfElse))
                {
                    g_scriptPtr--;
                    intptr_t *tempscrptr = g_scriptPtr;
                    g_warningCnt++;
                    C_ReportError(-1);

                    initprintf("%s:%d: warning: found `else' with no `if'.\n", g_scriptFileName, g_lineNumber);

                    if (C_GetKeyword() == CON_LEFTBRACE)
                    {
                        C_GetNextKeyword();
                        g_numBraces++;

                        C_ParseCommand(1);
                    }
                    else C_ParseCommand(0);

                    g_scriptPtr = tempscrptr;

                    continue;
                }

                intptr_t const lastScriptPtr = g_scriptPtr - apScript - 1;

                g_skipBranch = 0;
                g_checkingIfElse--;

                if (C_CheckMalformedBranch(lastScriptPtr))
                    continue;

                intptr_t const offset = (unsigned) (g_scriptPtr-apScript);

                g_scriptPtr++; //Leave a spot for the fail location

                if (!g_gotComment)
                    C_ParseCommand(0);

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                intptr_t *tempscrptr = (intptr_t *) apScript+offset;
                *tempscrptr = (intptr_t) g_scriptPtr;
                BITPTR_SET(tempscrptr-apScript);

                continue;
            }
        case CON_ADDLOGVAR:
            g_labelsOnly = 1;
            C_GetNextVar();
            g_labelsOnly = 0;
            continue;
        case CON_ADDVAR:
        case CON_SETVAR:
setvar:
        {
            auto ins = &g_scriptPtr[-1];

            C_GetNextVarType(GAMEVAR_READONLY);
            C_GetNextValue(LABEL_DEFINE);
            // replace instructions with special versions for specific var types
            scriptUpdateOpcodeForVariableType(ins);
            continue;
        }
        case CON_ADDVARVAR:
        case CON_SETVARVAR:
            {
//setvarvar:
                auto ins = &g_scriptPtr[-1];
                auto tptr = textptr;
                int const lnum = g_lineNumber;

                C_GetNextVarType(GAMEVAR_READONLY);
                C_GetNextVar();

                int const opcode = inthash_find(&h_varvar, *ins & VM_INSTMASK);

                if (ins[2] == GV_FLAG_CONSTANT && opcode != -1)
                {
                    if (g_scriptDebug > 1 && !g_errorCnt && !g_warningCnt)
                    {
                        initprintf("%s:%d: %s -> %s\n", g_scriptFileName, g_lineNumber,
                                    VM_GetKeywordForID(*ins & VM_INSTMASK), VM_GetKeywordForID(opcode));
                    }

                    tw = opcode;
                    scriptWriteAtOffset(opcode | LINE_NUMBER, ins);
                    g_scriptPtr = &ins[1];
                    textptr = tptr;
                    g_lineNumber = lnum;
                    goto setvar;
                }

                continue;
            }

        case CON_IFVARVARE:
        case CON_IFVARVARG:
        case CON_IFVARVARL:
            {
                auto const ins = &g_scriptPtr[-1];
                auto const lastScriptPtr = &g_scriptPtr[-1] - apScript;
                auto const lasttextptr = textptr;
                int const lnum = g_lineNumber;

                g_skipBranch = false;

                C_GetNextVar();
                auto const var = g_scriptPtr;
                C_GetNextVar();

                if (*var == GV_FLAG_CONSTANT)
                {
                    int const opcode = inthash_find(&h_varvar, tw);

                    if (opcode != -1)
                    {
                        if (g_scriptDebug > 1 && !g_errorCnt && !g_warningCnt)
                        {
                            initprintf("%s:%d: replacing %s with %s\n", g_scriptFileName, g_lineNumber,
                                       VM_GetKeywordForID(*ins & VM_INSTMASK), VM_GetKeywordForID(opcode));
                        }

                        scriptWriteAtOffset(opcode | LINE_NUMBER, ins);
                        tw = opcode;
                        g_scriptPtr = &ins[1];
                        textptr = lasttextptr;
                        g_lineNumber = lnum;
                        goto ifvar;
                    }
                }

                if (C_CheckMalformedBranch(lastScriptPtr))
                    continue;

                auto const offset = g_scriptPtr - apScript;
                g_scriptPtr++; // Leave a spot for the fail location

                C_ParseCommand(0);

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                auto const tempscrptr = apScript + offset;
                scriptWritePointer((intptr_t)g_scriptPtr, tempscrptr);
                continue;
            }

        case CON_IFVARE:
        case CON_IFVARG:
        case CON_IFVARL:
            {
ifvar:
                auto const ins = &g_scriptPtr[-1];
                auto const lastScriptPtr = &g_scriptPtr[-1] - apScript;

                g_skipBranch = false;

                C_GetNextVar();
                C_GetNextValue(LABEL_DEFINE);

                if (C_CheckMalformedBranch(lastScriptPtr))
                    continue;

                scriptUpdateOpcodeForVariableType(ins);

                auto const offset = g_scriptPtr - apScript;
                g_scriptPtr++; //Leave a spot for the fail location

                C_ParseCommand(0);

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                auto const tempscrptr = apScript + offset;
                scriptWritePointer((intptr_t)g_scriptPtr, tempscrptr);

                j = C_GetKeyword();

                if (j == CON_ELSE)
                    g_checkingIfElse++;

                continue;
            }

        case CON_IFRND:
        case CON_IFPDISTL:
        case CON_IFPDISTG:
        case CON_IFWASWEAPON:
        case CON_IFACTIONCOUNT:
        case CON_IFCOUNT:
        case CON_IFACTOR:
        case CON_IFSTRENGTH:
        case CON_IFSPAWNEDBY:
        case CON_IFGAPZL:
        case CON_IFFLOORDISTL:
        case CON_IFCEILINGDISTL:
        case CON_IFPHEALTHL:
        case CON_IFSPRITEPAL:
        case CON_IFGOTWEAPONCE:
        case CON_IFANGDIFFL:
        case CON_IFACTORHEALTHG:
        case CON_IFACTORHEALTHL:
        case CON_IFSOUNDID:
        case CON_IFSOUNDDIST:
        case CON_IFAI:
        case CON_IFACTION:
        case CON_IFMOVE:
        case CON_IFP:
        case CON_IFPINVENTORY:
            {
                intptr_t offset;
                intptr_t lastScriptPtr = (g_scriptPtr-&apScript[0]-1);

                g_skipBranch = 0;

                switch (tw)
                {
                case CON_IFAI:
                    C_GetNextValue(LABEL_AI);
                    break;
                case CON_IFACTION:
                    C_GetNextValue(LABEL_ACTION);
                    break;
                case CON_IFMOVE:
                    if (EDUKE32_PREDICT_FALSE((C_GetNextValue(LABEL_MOVE|LABEL_DEFINE) == 0) && (*(g_scriptPtr-1) != 0) && (*(g_scriptPtr-1) != 1)))
                    {
                        C_ReportError(-1);
                        *(g_scriptPtr-1) = 0;
                        initprintf("%s:%d: warning: expected a move, found a constant.\n",g_scriptFileName,g_lineNumber);
                        g_warningCnt++;
                    }
                    break;
                case CON_IFPINVENTORY:
                    C_GetNextValue(LABEL_DEFINE);
                    C_GetNextValue(LABEL_DEFINE);
                    break;
                case CON_IFP:
                    j = 0;
                    do
                        C_BitOrNextValue(&j);
                    while (C_GetKeyword() == -1);
                    C_FinishBitOr(j);
                    break;
                default:
                    C_GetNextValue(LABEL_DEFINE);
                    break;
                }

                if (C_CheckMalformedBranch(lastScriptPtr))
                    continue;

                intptr_t *tempscrptr = g_scriptPtr;
                offset = (unsigned)(tempscrptr-apScript);

                g_scriptPtr++; //Leave a spot for the fail location

                C_ParseCommand(0);

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                tempscrptr = (intptr_t *)apScript+offset;
                *tempscrptr = (intptr_t) g_scriptPtr;
                BITPTR_SET(tempscrptr-apScript);

                j = C_GetKeyword();

                if (j == CON_ELSE || j == CON_LEFTBRACE)
                    g_checkingIfElse++;

                continue;
            }

        case CON_IFONWATER:
        case CON_IFINWATER:
        case CON_IFACTORNOTSTAYPUT:
        case CON_IFCANSEE:
        case CON_IFHITWEAPON:
        case CON_IFSQUISHED:
        case CON_IFDEAD:
        case CON_IFCANSHOOTTARGET:
        case CON_IFHITSPACE:
        case CON_IFOUTSIDE:
        case CON_IFMULTIPLAYER:
        case CON_IFINSPACE:
        case CON_IFBULLETNEAR:
        case CON_IFRESPAWN:
        case CON_IFINOUTERSPACE:
        case CON_IFNOTMOVING:
        case CON_IFAWAYFROMWALL:
        case CON_IFCANSEETARGET:
        case CON_IFNOSOUNDS:
        case CON_IFNOCOVER:
        case CON_IFHITTRUCK:
        case CON_IFTIPCOW:
        case CON_IFONMUD:
        case CON_IFCOOP:
        case CON_IFMOTOFAST:
        case CON_IFWIND:
        case CON_IFONMOTO:
        case CON_IFONBOAT:
        case CON_IFSIZEDOWN:
        case CON_IFFINDNEWSPOT:
        case CON_IFPUPWIND:
            {
                intptr_t offset;
                intptr_t lastScriptPtr = (g_scriptPtr-&apScript[0]-1);

                g_skipBranch = 0;

                if (C_CheckMalformedBranch(lastScriptPtr))
                    continue;

                intptr_t *tempscrptr = g_scriptPtr;
                offset = (unsigned)(tempscrptr-apScript);

                g_scriptPtr++; //Leave a spot for the fail location

                C_ParseCommand(0);

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                tempscrptr = (intptr_t *)apScript+offset;
                *tempscrptr = (intptr_t) g_scriptPtr;
                BITPTR_SET(tempscrptr-apScript);

                j = C_GetKeyword();

                if (j == CON_ELSE || j == CON_LEFTBRACE)
                    g_checkingIfElse++;

                continue;
            }

        case CON_LEFTBRACE:
            if (EDUKE32_PREDICT_FALSE(!(g_processingState || g_parsingActorPtr || g_scriptEventOffset)))
            {
                g_errorCnt++;
                C_ReportError(ERROR_SYNTAXERROR);
            }
            g_numBraces++;

            C_ParseCommand(1);
            continue;

        case CON_RIGHTBRACE:
            g_numBraces--;

            if ((*(g_scriptPtr-2)>>12) == (IFELSE_MAGIC) &&
                ((*(g_scriptPtr-2) & VM_INSTMASK) == CON_LEFTBRACE)) // rewrite "{ }" into "nullop"
            {
                //            initprintf("%s:%d: rewriting empty braces '{ }' as 'nullop' from right\n",g_szScriptFileName,g_lineNumber);
                *(g_scriptPtr-2) = CON_NULLOP + (IFELSE_MAGIC<<12);
                g_scriptPtr -= 2;

                if (C_GetKeyword() != CON_ELSE && (*(g_scriptPtr-2) & VM_INSTMASK) != CON_ELSE)
                    g_skipBranch = 1;
                else g_skipBranch = 0;

                j = C_GetKeyword();

                if (g_checkingIfElse && j != CON_ELSE)
                    g_checkingIfElse--;

                return 1;
            }

            if (EDUKE32_PREDICT_FALSE(g_numBraces < 0))
            {
                C_ReportError(-1);
                initprintf("%s:%d: error: found more `}' than `{'.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
            }

            if (g_checkingIfElse && j != CON_ELSE)
                g_checkingIfElse--;

            return 1;

        case CON_BETANAME:
            g_scriptPtr--;
            j = 0;
            C_NextLine();
            continue;

        case CON_DEFINEVOLUMENAME:
            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;

            C_SkipSpace();

            if (EDUKE32_PREDICT_FALSE((unsigned)j > MAXVOLUMES-1))
            {
                initprintf("%s:%d: error: volume number exceeds maximum volume count.\n",
                    g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                C_NextLine();
                continue;
            }

			i = strcspn(textptr, "\r\n");
			gVolumeNames[j] = FStringTable::MakeMacro(textptr, i);
			textptr+=i;
            g_volumeCnt = j+1;
            continue;

        case CON_DEFINESKILLNAME:
            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;

            C_SkipSpace();

            if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXSKILLS))
            {
                initprintf("%s:%d: error: skill number exceeds maximum skill count %d.\n",
                           g_scriptFileName,g_lineNumber, MAXSKILLS);
                g_errorCnt++;
                C_NextLine();
                continue;
            }

			i = strcspn(textptr, "\r\n");
			gSkillNames[j] = FStringTable::MakeMacro(textptr, i);
			textptr+=i;

            for (i=0; i<MAXSKILLS; i++)
                if (gSkillNames[i].IsEmpty())
                    break;
            g_skillCnt = i;

            continue;

        case CON_DEFINELEVELNAME:
            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            j = *g_scriptPtr;
            C_GetNextValue(LABEL_DEFINE);
            g_scriptPtr--;
            k = *g_scriptPtr;
            C_SkipComments();

            if (EDUKE32_PREDICT_FALSE((unsigned)j > MAXVOLUMES-1))
            {
                initprintf("%s:%d: error: volume number exceeds maximum volume count.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                C_NextLine();
                continue;
            }
            if (EDUKE32_PREDICT_FALSE((unsigned)k > MAXLEVELS-1))
            {
                initprintf("%s:%d: error: level number exceeds maximum number of levels per episode.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                C_NextLine();
                continue;
            }

            i = 0;

            tempbuf[i] = '/';

            while (*textptr != ' ' && *textptr != '\t' && *textptr != 0x0a)
            {
                tempbuf[i+1] = *textptr;
                textptr++,i++;
                if (EDUKE32_PREDICT_FALSE(i >= BMAX_PATH))
                {
                    initprintf("%s:%d: error: level file name exceeds limit of %d characters.\n",g_scriptFileName,g_lineNumber,BMAX_PATH);
                    g_errorCnt++;
                    C_SkipSpace();
                    break;
                }
            }
            tempbuf[i+1] = '\0';

            Bcorrectfilename(tempbuf,0);

			mapList[j *MAXLEVELS+k].SetFileName(tempbuf);

            C_SkipComments();

            mapList[j *MAXLEVELS+k].parTime =
                (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*60)+
                (((*(textptr+3)-'0')*10+(*(textptr+4)-'0')));

            textptr += 5;
            C_SkipSpace();

            // cheap hack, 0.99 doesn't have the 3D Realms time
            if (*(textptr+2) == ':')
            {
                mapList[j *MAXLEVELS+k].designerTime =
                    (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*60)+
                    (((*(textptr+3)-'0')*10+(*(textptr+4)-'0')));

                textptr += 5;
                C_SkipSpace();
            }
            else if (g_scriptVersion == 10) g_scriptVersion = 9;

            i = 0;

            while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
            {
                tempbuf[i] = *textptr;
                textptr++,i++;
                if (EDUKE32_PREDICT_FALSE(i >= 32))
                {
                    initprintf("%s:%d: warning: truncating level name to %d characters.\n",
                        g_scriptFileName,g_lineNumber,32);
                    g_warningCnt++;
                    C_NextLine();
                    break;
                }
            }

            tempbuf[i] = '\0';

			mapList[j *MAXLEVELS+k].name = tempbuf;

            continue;

        case CON_DEFINEQUOTE:
		{
            g_scriptPtr--;

            C_GetNextValue(LABEL_DEFINE);

            k = *(g_scriptPtr-1);

            if (EDUKE32_PREDICT_FALSE((unsigned)k >= MAXQUOTES))
            {
                initprintf("%s:%d: error: quote number exceeds limit of %d.\n",g_scriptFileName,g_lineNumber,MAXQUOTES);
                g_errorCnt++;
            }
            else
            {
                C_AllocQuote(k);
            }

            g_scriptPtr--;

            i = 0;

            C_SkipSpace();

            buffer.Clear();
            while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
			{
				buffer.Push(*textptr);
				textptr++;
			}
			buffer.Push(0);
			quoteMgr.InitializeQuote(k, buffer.Data(), true);
			continue;
		}
        case CON_DEFINESOUND:
        {
            int ps, pe, vo, pr, m;

            g_scriptPtr--;
            C_GetNextValue(LABEL_DEFINE);

            // Ideally we could keep the value of i from C_GetNextValue() instead of having to hash_find() again.
            // This depends on tempbuf remaining in place after C_GetNextValue():
            j = hash_find(&h_labels, tempbuf);

            k = g_scriptPtr[-1];
            if ((unsigned)k >= MAXSOUNDS - 1)
            {
                initprintf("%s:%d: error: sound index exceeds limit of %d.\n", g_scriptFileName, g_lineNumber, MAXSOUNDS - 1);
                g_errorCnt++;
                k = MAXSOUNDS - 1;
            }
            /*else if (g_sounds[k].filename != NULL)
            {
                initprintf("%s:%d: warning: sound %d already defined (%s)\n", g_scriptFileName, g_lineNumber, k, g_sounds[k].filename);
                g_warningCnt++;
            }*/

            g_scriptPtr--;
            i = 0;
            C_SkipComments();

            buffer.Clear();

            if (*textptr == '\"')
            {
                textptr++;
                while (*textptr && *textptr != '\"')
                {
                    buffer.Push(*textptr++);
                }
                textptr++;
            }
            else while (*textptr != ' ' && *textptr != '\t' && *textptr != '\r' && *textptr != '\n')
            {
                buffer.Push(*textptr++);
            }
            buffer.Push(0);

            C_GetNextValue(LABEL_DEFINE);
            ps = g_scriptPtr[-1];
            C_GetNextValue(LABEL_DEFINE);
            pe = g_scriptPtr[-1];
            C_GetNextValue(LABEL_DEFINE);
            pr = g_scriptPtr[-1];

            C_GetNextValue(LABEL_DEFINE);
            m = g_scriptPtr[-1];

            C_GetNextValue(LABEL_DEFINE);
            vo = g_scriptPtr[-1];
            g_scriptPtr -= 5;

            int res = S_DefineSound(k, buffer.Data(), ps, pe, pr, m, vo, 1.f);

            continue;
        }

        case CON_ENDEVENT:

            if (EDUKE32_PREDICT_FALSE(!g_scriptEventOffset))
            {
                C_ReportError(-1);
                initprintf("%s:%d: error: found `endevent' without open `onevent'.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
            }
            if (EDUKE32_PREDICT_FALSE(g_numBraces != 0))
            {
                C_ReportError(g_numBraces > 0 ? ERROR_OPENBRACKET : ERROR_CLOSEBRACKET);
                g_errorCnt++;
            }

            g_scriptEventOffset = g_parsingActorPtr = 0;
            g_currentEvent = -1;
            Bsprintf(g_szCurrentBlockName,"(none)");
            continue;

        case CON_ENDA:
            if (EDUKE32_PREDICT_FALSE(!g_parsingActorPtr || g_scriptEventOffset))
            {
                C_ReportError(-1);
                initprintf("%s:%d: error: found `enda' without open `actor'.\n",g_scriptFileName,g_lineNumber);
                g_errorCnt++;
                g_scriptEventOffset = 0;
            }
            if (EDUKE32_PREDICT_FALSE(g_numBraces != 0))
            {
                C_ReportError(g_numBraces > 0 ? ERROR_OPENBRACKET : ERROR_CLOSEBRACKET);
                g_errorCnt++;
            }
            g_parsingActorPtr = 0;
            Bsprintf(g_szCurrentBlockName,"(none)");
            continue;

        case CON_BREAK:
            continue;

        case CON_FALL:
        case CON_TIP:
            //        case 21:
        case CON_KILLIT:
        case CON_RESETACTIONCOUNT:
        case CON_PSTOMP:
        case CON_RESETPLAYER:
        case CON_RESETCOUNT:
        case CON_WACKPLAYER:
        case CON_OPERATE:
        case CON_RESPAWNHITAG:
        case CON_GETLASTPAL:
        case CON_PKICK:
        case CON_MIKESND:
        case CON_TOSSWEAPON:
        case CON_DESTROYIT:
        case CON_LARRYBIRD:
        case CON_STRAFELEFT:
        case CON_STRAFERIGHT:
        case CON_SLAPPLAYER:
        case CON_TEARITUP:
        case CON_SMACKBUBBA:
        case CON_SOUNDTAGONCE:
        case CON_SOUNDTAG:
        case CON_SMACKSPRITE:
        case CON_FAKEBUBBA:
        case CON_MAMATRIGGER:
        case CON_MAMASPAWN:
        case CON_MAMAQUAKE:
        case CON_MAMAEND:
        case CON_GARYBANJO:
        case CON_MOTOLOOPSND:
        case CON_RNDMOVE:
        case CON_LEAVETRAX:
        case CON_LEAVEDROPPINGS:
        case CON_DEPLOYBIAS:
            continue;

        case CON_NULLOP:
            if (EDUKE32_PREDICT_FALSE(C_GetKeyword() != CON_ELSE))
            {
                C_ReportError(-1);
                g_warningCnt++;
                initprintf("%s:%d: warning: `nullop' found without `else'\n",g_scriptFileName,g_lineNumber);
                g_scriptPtr--;
                g_skipBranch = 1;
            }
            continue;

        case CON_GAMESTARTUP:
            {
                int32_t params[34];

                g_scriptPtr--;
                for (j = 0; j < 34; j++)
                {
                    C_GetNextValue(LABEL_DEFINE);
                    g_scriptPtr--;
                    params[j] = *g_scriptPtr;

                    if (j != 29 && j != 30) continue;

                    if (C_GetKeyword() != -1)
                    {
                        /*if (j == 12)
                            g_scriptVersion = 10;
                        else if (j == 21)
                            g_scriptVersion = 11;
                        else if (j == 25)
                            g_scriptVersion = 13;
                        else if (j == 29)
                            g_scriptVersion = 14;*/
                        break;
                    }
                    /*else
                        g_scriptVersion = 16;*/
                }

                /*
                v1.3d                   v1.5
                DEFAULTVISIBILITY       DEFAULTVISIBILITY
                GENERICIMPACTDAMAGE     GENERICIMPACTDAMAGE
                MAXPLAYERHEALTH         MAXPLAYERHEALTH
                STARTARMORHEALTH        STARTARMORHEALTH
                RESPAWNACTORTIME        RESPAWNACTORTIME
                RESPAWNITEMTIME         RESPAWNITEMTIME
                RUNNINGSPEED            RUNNINGSPEED
                RPGBLASTRADIUS          GRAVITATIONALCONSTANT
                PIPEBOMBRADIUS          RPGBLASTRADIUS
                SHRINKERBLASTRADIUS     PIPEBOMBRADIUS
                TRIPBOMBBLASTRADIUS     SHRINKERBLASTRADIUS
                MORTERBLASTRADIUS       TRIPBOMBBLASTRADIUS
                BOUNCEMINEBLASTRADIUS   MORTERBLASTRADIUS
                SEENINEBLASTRADIUS      BOUNCEMINEBLASTRADIUS
                MAXPISTOLAMMO           SEENINEBLASTRADIUS
                MAXSHOTGUNAMMO          MAXPISTOLAMMO
                MAXCHAINGUNAMMO         MAXSHOTGUNAMMO
                MAXRPGAMMO              MAXCHAINGUNAMMO
                MAXHANDBOMBAMMO         MAXRPGAMMO
                MAXSHRINKERAMMO         MAXHANDBOMBAMMO
                MAXDEVISTATORAMMO       MAXSHRINKERAMMO
                MAXTRIPBOMBAMMO         MAXDEVISTATORAMMO
                MAXFREEZEAMMO           MAXTRIPBOMBAMMO
                CAMERASDESTRUCTABLE     MAXFREEZEAMMO
                NUMFREEZEBOUNCES        MAXGROWAMMO
                FREEZERHURTOWNER        CAMERASDESTRUCTABLE
                NUMFREEZEBOUNCES
                FREEZERHURTOWNER
                QSIZE
                TRIPBOMBLASERMODE
                */

                G_DoGameStartup(params);
            }
            continue;
        }
    }
    while (loop);

    return 0;
}

static char const * C_ScriptVersionString(int32_t version)
{
    switch (version)
    {
    case 9:
        return ", v0.99 compatibility mode";
    case 10:
        return ", v1.0 compatibility mode";
    case 11:
        return ", v1.1 compatibility mode";
    case 13:
        return ", v1.3D compatibility mode";
    default:
        return "";
    }
}

void C_PrintStats(void)
{
    initprintf("%d/%d labels\n", g_labelCnt,
        (int32_t) min((MAXSECTORS * sizeof(sectortype)/sizeof(int32_t)),
            MAXSPRITES * sizeof(spritetype)/(1<<6)));

    int i, j;

    for (i=MAXTILES-1, j=0; i>=0; i--)
    {
        if (g_tile[i].execPtr)
            j++;
    }
    if (j) initprintf("%d actors", j);

    initprintf("\n");
}

void C_Compile(const char *fileName)
{
    Bmemset(apScriptEvents, 0, sizeof(apScriptEvents));
    Bmemset(apScriptGameEventEnd, 0, sizeof(apScriptGameEventEnd));

    for (int i=0; i<MAXTILES; i++)
    {
        Bmemset(&g_tile[i], 0, sizeof(tiledata_t));
        g_actorMinMs[i] = 1e308;
    }

    C_InitHashes();
    Gv_Init();

    auto kFile = fileSystem.OpenFileReader(fileName,0);

	if (!kFile.isOpen())
    {
		if (!kFile.isOpen())
		{
			I_Error(tempbuf, "CON file `%s' missing.", fileName);
		}
	}

    int const kFileLen = kFile.GetLength();

    initprintf("Compiling: %s (%d bytes)\n", fileName, kFileLen);

    uint32_t const startcompiletime = timerGetTicks();

    char * mptr = (char *)Xmalloc(kFileLen+1);
    mptr[kFileLen] = 0;

    textptr = (char *) mptr;
    kFile.Read((char *)textptr,kFileLen);
	kFile.Close();

    g_scriptcrc = Bcrc32(NULL, 0, 0L);
    g_scriptcrc = Bcrc32(textptr, kFileLen, g_scriptcrc);

    Xfree(apScript);

    apScript = (intptr_t *)Xcalloc(1, g_scriptSize * sizeof(intptr_t));
    bitptr   = (char *)Xcalloc(1, (((g_scriptSize + 7) >> 3) + 1) * sizeof(uint8_t));
    //    initprintf("script: %d, bitptr: %d\n",script,bitptr);

    g_labelCnt        = 0;
    g_defaultLabelCnt = 0;
    g_scriptPtr       = apScript + 3;  // move permits constants 0 and 1; moveptr[1] would be script[2] (reachable?)
    g_warningCnt      = 0;
    g_errorCnt        = 0;
    g_lineNumber      = 1;
    g_totalLines      = 0;

    Bstrcpy(g_scriptFileName, fileName);

    C_ParseCommand(1);

    if (userConfig.AddCons) for (FString & m : *userConfig.AddCons.get())
    {
		C_Include(m);
    }
	userConfig.AddCons.reset();
    
    if (g_errorCnt > 63)
        initprintf("fatal error: too many errors: Aborted\n");

    //*script = (intptr_t) g_scriptPtr;

    DO_FREE_AND_NULL(mptr);

    if (g_warningCnt || g_errorCnt)
        initprintf("Found %d warning(s), %d error(s).\n", g_warningCnt, g_errorCnt);

    if (g_errorCnt)
    {
        Bsprintf(buf, "Error compiling CON files.");
        G_GameExit(buf);
    }

    g_totalLines += g_lineNumber;

    C_SetScriptSize(g_scriptPtr-apScript+8);

    initprintf("Script compiled in %dms, %ld bytes%s\n", timerGetTicks() - startcompiletime,
                (unsigned long)(g_scriptPtr-apScript), C_ScriptVersionString(g_scriptVersion));

    for (auto *i : tables_free)
        hash_free(i);

    freesoundhashnames();

    if (g_scriptDebug)
        C_PrintStats();

    C_InitQuotes();
}

void C_ReportError(int32_t iError)
{
    if (Bstrcmp(g_szCurrentBlockName,g_szLastBlockName))
    {
        if (g_scriptEventOffset || g_processingState || g_parsingActorPtr)
            initprintf("%s: In %s `%s':\n",g_scriptFileName,g_scriptEventOffset?"event":g_parsingActorPtr?"actor":"state",g_szCurrentBlockName);
        else initprintf("%s: At top level:\n",g_scriptFileName);
        Bstrcpy(g_szLastBlockName,g_szCurrentBlockName);
    }
    switch (iError)
    {
    case ERROR_CLOSEBRACKET:
        initprintf("%s:%d: error: found more `}' than `{' before `%s'.\n",g_scriptFileName,g_lineNumber,tempbuf);
        break;
    case ERROR_EXCEEDSMAXTILES:
        initprintf("%s:%d: error: `%s' value exceeds MAXTILES.  Maximum is %d.\n",g_scriptFileName,g_lineNumber,tempbuf,MAXTILES-1);
        break;
    case ERROR_EXPECTEDKEYWORD:
        initprintf("%s:%d: error: expected a keyword but found `%s'.\n",g_scriptFileName,g_lineNumber,tempbuf);
        break;
    case ERROR_FOUNDWITHIN:
        initprintf("%s:%d: error: found `%s' within %s.\n",g_scriptFileName,g_lineNumber,tempbuf,g_parsingActorPtr?"an actor":"a state");
        break;
    case ERROR_ISAKEYWORD:
        initprintf("%s:%d: error: symbol `%s' is a keyword.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
        break;
    case ERROR_OPENBRACKET:
        initprintf("%s:%d: error: found more `{' than `}' before `%s'.\n",g_scriptFileName,g_lineNumber,tempbuf);
        break;
    case ERROR_NOTAGAMEVAR:
        initprintf("%s:%d: error: symbol `%s' is not a variable.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case ERROR_PARAMUNDEFINED:
        initprintf("%s:%d: error: parameter `%s' is undefined.\n",g_scriptFileName,g_lineNumber,tempbuf);
        break;
    case ERROR_SYNTAXERROR:
        initprintf("%s:%d: error: syntax error.\n",g_scriptFileName,g_lineNumber);
        break;
    case ERROR_VARREADONLY:
        initprintf("%s:%d: error: variable `%s' is read-only.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case ERROR_VARTYPEMISMATCH:
        initprintf("%s:%d: error: variable `%s' is of the wrong type.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case WARNING_BADGAMEVAR:
        initprintf("%s:%d: warning: variable `%s' should be either per-player OR per-actor, not both.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case WARNING_DUPLICATEDEFINITION:
        initprintf("%s:%d: warning: duplicate definition `%s' ignored.\n",g_scriptFileName,g_lineNumber,LAST_LABEL);
        break;
    case WARNING_LABELSONLY:
        initprintf("%s:%d: warning: expected a label, found a constant.\n",g_scriptFileName,g_lineNumber);
        break;
    case WARNING_VARMASKSKEYWORD:
        initprintf("%s:%d: warning: variable `%s' masks keyword.\n", g_scriptFileName, g_lineNumber, LAST_LABEL);
        break;
    }
}

END_RR_NS
