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

#include "concmd.h"

#include "duke3d_ed.h"
#include "namesdyn.h"
#include "gamedef.h"
#include "gameexec.h"
#include "savegame.h"
#include "common.h"
#include "common_game.h"
#include "cheats.h"
#include "m_argv.h"

#include "osd.h"
#include "m_crc32.h"
#include "printf.h"
#include "menu/menu.h"
#include "stringtable.h"
#include "mapinfo.h"

BEGIN_DUKE_NS


void SortCommands();
int getkeyword(const char* text);
void skiptoendofline();
void skipwhitespace();
void skipblockcomment();
bool skipcomments();
int findlabel(const char* text);


#define LINE_NUMBER (line_number << 12)

int32_t g_scriptVersion = 14; // 13 = 1.3D-style CON files, 14 = 1.4/1.5 style CON files

char g_scriptFileName[BMAX_PATH] = "(none)";  // file we're currently compiling

int32_t g_totalLines;
uint32_t g_scriptcrc;
char g_szBuf[1024];

static char g_szCurrentBlockName[256] = "(none)", g_szLastBlockName[256] = "NULL";
static int32_t g_lastKeyword = -1;
extern int checking_ifelse;
extern int parsing_state;

// The pointer to the start of the case table in a switch statement.
// First entry is 'default' code.
static intptr_t *g_caseScriptPtr;
static int32_t g_labelsOnly = 0;
extern int num_squigilly_brackets;

int32_t C_ParseCommand(int32_t loop);
static int32_t C_SetScriptSize(int32_t size);

static intptr_t apScriptGameEventEnd[MAXEVENTS];
extern intptr_t parsing_actor;
static intptr_t g_scriptEventOffset;
extern char *textptr;


static const vec2_t varvartable[] =
{
    { concmd_ifvarvare,         concmd_ifvare },
    { concmd_ifvarvarg,         concmd_ifvarg },
    { concmd_ifvarvarl,         concmd_ifvarl },
    { concmd_addvarvar,         concmd_addvar },
    { concmd_setvarvar,         concmd_setvar },
};

static inthashtable_t h_varvar = { NULL, INTHASH_SIZE(ARRAY_SIZE(varvartable)) };

static inthashtable_t *const inttables[] = {
    &h_varvar,
};

char const * VM_GetKeywordForID(int32_t id)
{
    // do not really need this for now...
    return "<invalid keyword>";
}

#define BITPTR_SET(x)
#define BITPTR_CLEAR(x)
#define BITPTR_IS_POINTER(x)

hashtable_t h_gamevars = { MAXGAMEVARS >> 1, NULL };

static hashtable_t * const tables[] = {
    &h_gamevars
};

void C_InitHashes()
{
    SortCommands();
    for (auto table : tables)
        hash_init(table);

    for (auto table : inttables)
        inthash_init(table);

    for (auto &varvar : varvartable)
        inthash_add(&h_varvar, varvar.x, varvar.y, 0);

    //inithashnames();
    initsoundhashnames();
}

// "magic" number for { and }, overrides line number in compiled code for later detection
#define IFELSE_MAGIC 31337
static int32_t g_skipBranch;

static int32_t C_SetScriptSize(int32_t newsize)
{
    return 0;
}

extern bool ispecial(const char c);

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
            line_number++;
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
                if (!(errorcount || warningcount) && g_scriptDebug > 1)
                    Printf("%s:%d: debug: got comment.\n",g_scriptFileName,line_number);
                skiptoendofline();
                g_gotComment = 1;
                continue;
            case '*': // beginning of a C style comment
                if (!(errorcount || warningcount) && g_scriptDebug > 1)
                    Printf("%s:%d: debug: got start of comment block.\n",g_scriptFileName,line_number);
                do
                {
                    if (*textptr == '\n')
                        line_number++;
                    textptr++;
                }
                while (*textptr && (textptr[0] != '*' || textptr[1] != '/'));

                if (EDUKE32_PREDICT_FALSE(!*textptr))
                {
                    if (!(errorcount || warningcount) && g_scriptDebug)
                        Printf("%s:%d: debug: EOF in comment!\n",g_scriptFileName,line_number);
                    C_ReportError(-1);
                    Printf("%s:%d: error: found `/*' with no `*/'.\n",g_scriptFileName,line_number);
                    parsing_actor = parsing_state = num_squigilly_brackets = 0;
                    errorcount++;
                    continue;
                }

                if (!(errorcount || warningcount) && g_scriptDebug > 1)
                    Printf("%s:%d: debug: got end of comment block.\n",g_scriptFileName,line_number);

                textptr+=2;
                g_gotComment = 1;
                continue;
            default:
                C_ReportError(-1);
                Printf("%s:%d: error: malformed comment.\n", g_scriptFileName, line_number);
                skiptoendofline();
                errorcount++;
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
            return ((scriptptr-apScript) > (g_scriptSize-32)) ? C_SetScriptSize(g_scriptSize<<1) : 0;
        }
    }
    while (1);
}

static inline int GetDefID(char const *label) { return hash_find(&h_gamevars, label); }

#define LAST_LABEL (label+(labelcnt<<6))
bool isaltok(const char c);

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
        label[(labelcnt<<6)+(i++)] = *(textptr++);

    label[(labelcnt<<6)+i] = 0;

    if (!(errorcount|warningcount) && g_scriptDebug > 1)
        Printf("%s:%d: debug: label `%s'.\n",g_scriptFileName,line_number,label+(labelcnt<<6));
}

void scriptWriteValue(int32_t const value)
{
    BITPTR_CLEAR(scriptptr-apScript);
    *scriptptr++ = value;
}

// addresses passed to these functions must be within the block of memory pointed to by apScript
void scriptWriteAtOffset(int32_t const value, intptr_t * const addr)
{
    BITPTR_CLEAR(addr-apScript);
    *(addr) = value;
}

void scriptWritePointer(intptr_t const value, intptr_t * const addr)
{
    BITPTR_SET(addr-apScript);
    *(addr) = value;
}

// addresses passed to these functions must be within the block of memory pointed to by apScript
void scriptWriteAtOffset(int32_t const value, intptr_t addr)
{
    BITPTR_CLEAR(addr);
    apScript[addr] = value;
}

void scriptWritePointer(intptr_t const value, intptr_t addr)
{
    BITPTR_SET(addr);
    apScript[addr] = value;
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

    return getkeyword(tempbuf);
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

    if ((i=getkeyword(tempbuf)) >= 0)
    {
        if (i == concmd_leftbrace || i == concmd_rightbrace || i == concmd_nullop)
            scriptWriteValue(i | (VM_IFELSE_MAGIC<<12));
        else scriptWriteValue(i | LINE_NUMBER);

        textptr += l;

        if (!(errorcount || warningcount) && g_scriptDebug)
            Printf("%s:%d: debug: keyword `%s'.\n", g_scriptFileName, line_number, tempbuf);
        return i;
    }

    textptr += l;
    errorcount++;

    if (EDUKE32_PREDICT_FALSE((tempbuf[0] == '{' || tempbuf[0] == '}') && tempbuf[1] != 0))
    {
        C_ReportError(-1);
        Printf("%s:%d: error: expected whitespace between `%c' and `%s'.\n",g_scriptFileName,line_number,tempbuf[0],tempbuf+1);
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
        Printf("%s:%d: warning: number greater than INT32_MAX converted to a negative one.\n",
                   g_szScriptFileName,line_number);
        g_numCompilerWarnings++;
#endif
    }
    else
    {
        // out of range, this is arguably worse

        Printf("%s:%d: warning: number out of the range of a 32-bit integer encountered.\n",
                   g_scriptFileName,line_number);
        warningcount++;
    }

    return (int32_t)num;
}

static int32_t parse_hex_constant(const char *hexnum)
{
    uint64_t x;
    sscanf(hexnum, "%" PRIx64 "", &x);

    if (EDUKE32_PREDICT_FALSE(x > UINT32_MAX))
    {
        Printf(g_scriptFileName, ":", line_number, ": warning: number 0x", hex(x), " truncated to 32 bits.\n");
        warningcount++;
    }

    return x;
}

static void C_GetNextVarType(int32_t type)
{
    int32_t id    = 0;
    int32_t flags = 0;

    auto varptr = scriptptr;

    C_SkipComments();

    if (!type && !g_labelsOnly && (isdigit(*textptr) || ((*textptr == '-') && (isdigit(*(textptr+1))))))
    {
        scriptWriteValue(GV_FLAG_CONSTANT);

        if (tolower(textptr[1])=='x')  // hex constants
            scriptWriteValue(parse_hex_constant(textptr+2));
        else
            scriptWriteValue(parse_decimal_number());

        if (!(errorcount || warningcount) && g_scriptDebug)
            Printf("%s:%d: debug: constant %ld in place of gamevar.\n", g_scriptFileName, line_number, (long)(scriptptr[-1]));
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
            errorcount++;
            C_ReportError(ERROR_SYNTAXERROR);
            C_GetNextLabelName();
            return;
        }

        if (!(errorcount || warningcount) && g_scriptDebug)
            Printf("%s:%d: debug: flagging gamevar as negative.\n", g_scriptFileName, line_number); //,Batol(textptr));

        flags = GV_FLAG_NEGATIVE;
        textptr++;
    }

    C_GetNextLabelName();

    if (getkeyword(LAST_LABEL)>=0)
    {
        errorcount++;
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
            id = findlabel(tempbuf);

            if (EDUKE32_PREDICT_TRUE(id>=0 /*&& labeltype[id] & LABEL_DEFINE*/))
            {
                if (!(errorcount || warningcount) && g_scriptDebug)
                    Printf("%s:%d: debug: label `%s' in place of gamevar.\n",g_scriptFileName,line_number,label+(id<<6));

                scriptWriteValue(GV_FLAG_CONSTANT);
                scriptWriteValue(labelcode[id]);
                return;
            }
        }

        errorcount++;
        C_ReportError(ERROR_NOTAGAMEVAR);
        return;
    }

    if (EDUKE32_PREDICT_FALSE(type == GAMEVAR_READONLY && aGameVars[id].flags & GAMEVAR_READONLY))
    {
        errorcount++;
        C_ReportError(ERROR_VARREADONLY);
        return;
    }
    else if (EDUKE32_PREDICT_FALSE(aGameVars[id].flags & type))
    {
        errorcount++;
        C_ReportError(ERROR_VARTYPEMISMATCH);
        return;
    }

    if (g_scriptDebug > 1 && !errorcount && !warningcount)
        Printf("%s:%d: debug: gamevar `%s'.\n",g_scriptFileName,line_number,LAST_LABEL);

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
// *scriptptr will contain the value OR 0 if wrong type or error
#define C_GetNextValue(a) C_GetNextValue_()
static int32_t C_GetNextValue_()
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

    if (getkeyword(tempbuf) >= 0)
    {
        errorcount++;
        C_ReportError(ERROR_ISAKEYWORD);
        return -1;
    }

    int32_t i = findlabel(tempbuf);

    if (i>=0)
    {
        //if (EDUKE32_PREDICT_TRUE(labeltype[i] & type))
        {

            BITPTR_CLEAR(scriptptr-apScript);
            *(scriptptr++) = labelcode[i];

            textptr += l;
            return 0;// labeltype[i];
        }
    }

    if (EDUKE32_PREDICT_FALSE(isdigit(*textptr) == 0 && *textptr != '-'))
    {
        C_ReportError(ERROR_PARAMUNDEFINED);
        errorcount++;
        BITPTR_CLEAR(scriptptr-apScript);
        *scriptptr = 0;
        scriptptr++;
        textptr+=l;
        if (!l) textptr++;
        return -1; // error!
    }

    if (EDUKE32_PREDICT_FALSE(isdigit(*textptr) && g_labelsOnly))
    {
        C_ReportError(WARNING_LABELSONLY);
        warningcount++;
    }

    i = l-1;
    do
    {
        // FIXME: check for 0-9 A-F for hex
        if (textptr[0] == '0' && textptr[1] == 'x') break; // kill the warning for hex
        if (EDUKE32_PREDICT_FALSE(!isdigit(textptr[i--])))
        {
            C_ReportError(-1);
            Printf("%s:%d: warning: invalid character `%c' in definition!\n",g_scriptFileName,line_number,textptr[i+1]);
            warningcount++;
            break;
        }
    }
    while (i > 0);

    BITPTR_CLEAR(scriptptr-apScript);

    if (textptr[0] == '0' && tolower(textptr[1])=='x')
        *scriptptr = parse_hex_constant(textptr+2);
    else
        *scriptptr = parse_decimal_number();

    if (!(errorcount || warningcount) && g_scriptDebug > 1)
        Printf("%s:%d: debug: constant %ld.\n",
                   g_scriptFileName,line_number,(long)*scriptptr);

    scriptptr++;

    textptr += l;

    return 0;   // literal value
}

static int32_t C_CheckMalformedBranch(intptr_t lastScriptPtr)
{
    switch (C_GetKeyword())
    {
    case concmd_rightbrace:
    case concmd_enda:
    case concmd_ends:
    case concmd_else:
        scriptptr = lastScriptPtr + &apScript[0];
        g_skipBranch = 1;
        C_ReportError(-1);
        warningcount++;
        Printf("%s:%d: warning: malformed `%s' branch\n",g_scriptFileName,line_number,
                   VM_GetKeywordForID(*(scriptptr) & VM_INSTMASK));
        return 1;
    }
    return 0;
}

static int32_t C_CheckEmptyBranch(int32_t tw, intptr_t lastScriptPtr)
{
    // ifrnd and the others actually do something when the condition is executed
    if ((Bstrncmp(VM_GetKeywordForID(tw), "if", 2) && tw != concmd_else) ||
            tw == concmd_ifrnd || tw == concmd_ifhitweapon || tw == concmd_ifcansee || tw == concmd_ifcanseetarget ||
            tw == concmd_ifpdistl || tw == concmd_ifpdistg || tw == concmd_ifgotweaponce)
    {
        g_skipBranch = 0;
        return 0;
    }

    if ((*(scriptptr) & VM_INSTMASK) != concmd_nullop || *(scriptptr)>>12 != IFELSE_MAGIC)
        g_skipBranch = 0;

    if (EDUKE32_PREDICT_FALSE(g_skipBranch))
    {
        C_ReportError(-1);
        warningcount++;
        scriptptr = lastScriptPtr + &apScript[0];
        Printf("%s:%d: warning: empty `%s' branch\n",g_scriptFileName,line_number,
                   VM_GetKeywordForID(*(scriptptr) & VM_INSTMASK));
        *(scriptptr) = (concmd_nullop + (IFELSE_MAGIC<<12));
        return 1;
    }
    return 0;
}

extern int g_currentSourceFile;
static void C_Include(const char *confile)
{
	auto fp = fileSystem.OpenFileReader(confile);

    if (!fp.isOpen())
    {
        errorcount++;
        Printf("%s:%d: error: could not find file `%s'.\n",g_scriptFileName,line_number,confile);
        return;
    }
    g_currentSourceFile = fileSystem.FindFile(confile);

	int32_t j = fp.GetLength();

    char *mptr = (char *)Xmalloc(j+1);

    Printf("Including: %s (%d bytes)\n",confile, j);

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

    int32_t temp_ScriptLineNumber = line_number;
    line_number = 1;

    int32_t temp_ifelse_check = checking_ifelse;
    checking_ifelse = 0;

    textptr = mptr;

    C_SkipComments();
    C_ParseCommand(1);

    Bstrcpy(g_scriptFileName, parentScriptFileName);

    g_totalLines += line_number;
    line_number = temp_ScriptLineNumber;
    checking_ifelse = temp_ifelse_check;

    textptr = origtptr;

    Xfree(mptr);
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
    scriptptr--;
    *valptr |= *scriptptr;
}

static inline void C_FinishBitOr(int32_t value)
{
    BITPTR_CLEAR(scriptptr-apScript);
    *scriptptr++ = value;
}

int parsecommand(int tw); // for now just run an externally parsed command.

int32_t C_ParseCommand(int32_t loop)
{
    int32_t i, j=0, k=0, tw;
    TArray<char> buffer;

    do
    {
        if (EDUKE32_PREDICT_FALSE(errorcount > 63 || (*textptr == '\0') || (*(textptr+1) == '\0') || C_SkipComments()))
            return 1;

        if (EDUKE32_PREDICT_FALSE(g_scriptDebug))
            C_ReportError(-1);

        switch ((g_lastKeyword = tw = C_GetNextKeyword()))
        {
        default:
        case -1:
        case -2:
            return 1; //End
        case concmd_state:
        case concmd_ends:
        case concmd_define:
        case concmd_palfrom:
        case concmd_move:
        case concmd_music:
        case concmd_ai:
        case concmd_action:
        case concmd_actor:
        case concmd_useractor:
        case concmd_cstat:
        case concmd_strength:
        case concmd_shoot:
        case concmd_addphealth:
        case concmd_spawn:
        case concmd_count:
        case concmd_endofgame:
        case concmd_spritepal:
        case concmd_cactor:
        case concmd_money:
        case concmd_addkills:
        case concmd_debug:
        case concmd_addstrength:
        case concmd_cstator:
        case concmd_mail:
        case concmd_paper:
        case concmd_sleeptime:
        case concmd_clipdist:
        case concmd_isdrunk:
        case concmd_iseat:
        case concmd_newpic:
        case concmd_hitradius:
        case concmd_addammo:
        case concmd_addweapon:
        case concmd_sizeto:
        case concmd_sizeat:
        case concmd_debris:
        case concmd_addinventory:
        case concmd_guts:
        case concmd_lotsofglass:
        case concmd_quote:
        case concmd_sound:
        case concmd_globalsound:
        case concmd_soundonce:
        case concmd_stopsound:
        case concmd_ifrnd:
        case concmd_ifpdistl:
        case concmd_ifpdistg:
        case concmd_ifai:
        case concmd_ifwasweapon:
        case concmd_ifaction:
        case concmd_ifactioncount:
        case concmd_ifmove:
        case concmd_ifcount:
        case concmd_ifactor:
        case concmd_ifstrength:
        case concmd_ifspawnedby:
        case concmd_ifgapzl:
        case concmd_iffloordistl:
        case concmd_ifceilingdistl:
        case concmd_ifphealthl:
        case concmd_ifspritepal:
        case concmd_ifgotweaponce:
        case concmd_ifangdiffl:
        case concmd_ifactorhealthg:
        case concmd_ifactorhealthl:
        case concmd_ifsoundid:
        case concmd_ifsounddist:
        case concmd_ifpinventory:
        case concmd_ifonwater:
        case concmd_ifinwater:
        case concmd_ifactornotstayput:
        case concmd_ifcansee:
        case concmd_ifhitweapon:
        case concmd_ifsquished:
        case concmd_ifdead:
        case concmd_ifcanshoottarget:
        case concmd_ifp:
        case concmd_ifhitspace:
        case concmd_ifoutside:
        case concmd_ifmultiplayer:
        case concmd_ifinspace:
        case concmd_ifbulletnear:
        case concmd_ifrespawn:
        case concmd_ifinouterspace:
        case concmd_ifnotmoving:
        case concmd_ifawayfromwall:
        case concmd_ifcanseetarget:
        case concmd_ifnosounds:
        case concmd_ifnocover:
        case concmd_ifhittruck:
        case concmd_iftipcow:
        case concmd_ifonmud:
        case concmd_ifcoop:
        case concmd_ifmotofast:
        case concmd_ifwind:
        case concmd_ifonmoto:
        case concmd_ifonboat:
        case concmd_ifsizedown:
        case concmd_ifplaybackon:
        case concmd_else:
        case concmd_leftbrace:
        case concmd_rightbrace:
        case concmd_betaname:
        case concmd_definevolumename:
        case concmd_defineskillname:
        case concmd_definelevelname:
        case concmd_definequote:
        case concmd_definesound:
        case concmd_enda:
        case concmd_include:
        case concmd_break:
        case concmd_fall:
        case concmd_tip:
        case concmd_killit:
        case concmd_resetactioncount:
        case concmd_pstomp:
        case concmd_resetplayer:
        case concmd_resetcount:
        case concmd_wackplayer:
        case concmd_operate:
        case concmd_respawnhitag:
        case concmd_getlastpal:
        case concmd_pkick:
        case concmd_mikesnd:
        case concmd_tossweapon:
        case concmd_destroyit:
        case concmd_larrybird:
        case concmd_strafeleft:
        case concmd_straferight:
        case concmd_slapplayer:
        case concmd_tearitup:
        case concmd_smackbubba:
        case concmd_soundtagonce:
        case concmd_soundtag:
        case concmd_smacksprite:
        case concmd_fakebubba:
        case concmd_mamatrigger:
        case concmd_mamaspawn:
        case concmd_mamaquake:
        case concmd_mamaend:
        case concmd_garybanjo:
        case concmd_motoloopsnd:
        case concmd_rndmove:
        case concmd_gamestartup:
            if (parsecommand(g_lastKeyword)) return 1;
            continue;

        case concmd_gamevar:
        {
            // syntax: gamevar <var1> <initial value> <flags>
            // defines var1 and sets initial value.
            // flags are used to define usage
            // (see top of this files for flags)

            if (EDUKE32_PREDICT_FALSE(isdigit(*textptr) || (*textptr == '-')))
            {
                errorcount++;
                C_ReportError(ERROR_SYNTAXERROR);
                skiptoendofline();
                continue;
            }

            scriptptr--;

            C_GetNextLabelName();

            if (getkeyword(label + (labelcnt << 6)) >= 0)
            {
                errorcount++;
                C_ReportError(WARNING_VARMASKSKEYWORD);
                continue;
            }

            int32_t defaultValue = 0;
            int32_t varFlags     = 0;

            if (C_GetKeyword() == -1)
            {
                C_GetNextValue(LABEL_DEFINE); // get initial value
                defaultValue = *(--scriptptr);

                j = 0;

                while (C_GetKeyword() == -1)
                    C_BitOrNextValue(&j);

                C_FinishBitOr(j);
                varFlags = *(--scriptptr);

                if (EDUKE32_PREDICT_FALSE((*(scriptptr)&GAMEVAR_USER_MASK)==(GAMEVAR_PERPLAYER|GAMEVAR_PERACTOR)))
                {
                    warningcount++;
                    varFlags ^= GAMEVAR_PERPLAYER;
                    C_ReportError(WARNING_BADGAMEVAR);
                }
            }

            Gv_NewVar(LAST_LABEL, defaultValue, varFlags);
            continue;
        }



		case concmd_onevent:
            if (EDUKE32_PREDICT_FALSE(parsing_state || parsing_actor))
            {
                C_ReportError(ERROR_FOUNDWITHIN);
                errorcount++;
            }

            num_squigilly_brackets = 0;
            scriptptr--;
            g_scriptEventOffset = parsing_actor = scriptptr - apScript;

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
            scriptptr--;
            j= *scriptptr;  // type of event
            g_currentEvent = j;
            //Bsprintf(g_szBuf,"Adding Event for %d at %lX",j, g_parsingEventPtr);
            //AddLog(g_szBuf);
            if (EDUKE32_PREDICT_FALSE((unsigned)j > MAXEVENTS-1))
            {
                Printf("%s:%d: error: invalid event ID.\n",g_scriptFileName,line_number);
                errorcount++;
                continue;
            }
            // if event has already been declared then store previous script location
            apScriptEvents[j] = g_scriptEventOffset;

            checking_ifelse = 0;

            continue;

        case concmd_addlogvar:
            g_labelsOnly = 1;
            C_GetNextVar();
            g_labelsOnly = 0;
            continue;
        case concmd_setvar:
        case concmd_addvar:
    setvar:
        {
            auto ins = &scriptptr[-1];

            C_GetNextVarType(GAMEVAR_READONLY);
            C_GetNextValue(LABEL_DEFINE);
            continue;
        }
        case concmd_setvarvar:
        case concmd_addvarvar:
            {
//setvarvar:
                auto ins = &scriptptr[-1];
                auto tptr = textptr;
                int const lnum = line_number;

                C_GetNextVarType(GAMEVAR_READONLY);
                C_GetNextVar();

                int const opcode = inthash_find(&h_varvar, *ins & VM_INSTMASK);

                if (ins[2] == GV_FLAG_CONSTANT && opcode != -1)
                {
                    if (g_scriptDebug > 1 && !errorcount && !warningcount)
                    {
                        Printf("%s:%d: %s -> %s\n", g_scriptFileName, line_number,
                                    VM_GetKeywordForID(*ins & VM_INSTMASK), VM_GetKeywordForID(opcode));
                    }

                    tw = opcode;
                    scriptWriteAtOffset(opcode | LINE_NUMBER, ins);
                    scriptptr = &ins[1];
                    textptr = tptr;
                    line_number = lnum;
                    goto setvar;
                }

                continue;
            }

        case concmd_ifvarvarg:
        case concmd_ifvarvarl:
        case concmd_ifvarvare:
            {
                auto const ins = &scriptptr[-1];
                auto const lastScriptPtr = &scriptptr[-1] - apScript;
                auto const lasttextptr = textptr;
                int const lnum = line_number;

                g_skipBranch = false;

                C_GetNextVar();
                auto const var = scriptptr;
                C_GetNextVar();

                if (*var == GV_FLAG_CONSTANT)
                {
                    int const opcode = inthash_find(&h_varvar, tw);

                    if (opcode != -1)
                    {
                        if (g_scriptDebug > 1 && !errorcount && !warningcount)
                        {
                            Printf("%s:%d: replacing %s with %s\n", g_scriptFileName, line_number,
                                       VM_GetKeywordForID(*ins & VM_INSTMASK), VM_GetKeywordForID(opcode));
                        }

                        scriptWriteAtOffset(opcode | LINE_NUMBER, ins);
                        tw = opcode;
                        scriptptr = &ins[1];
                        textptr = lasttextptr;
                        line_number = lnum;
                        goto ifvar;
                    }
                }

                if (C_CheckMalformedBranch(lastScriptPtr))
                    continue;

                auto const offset = scriptptr - apScript;
                scriptptr++; // Leave a spot for the fail location

                C_ParseCommand(0);

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                auto const tempscrptr = apScript + offset;
                scriptWritePointer((intptr_t)scriptptr, tempscrptr);
                continue;
            }

        case concmd_ifvarl:
        case concmd_ifvarg:
        case concmd_ifvare:
            {
ifvar:
                auto const ins = &scriptptr[-1];
                auto const lastScriptPtr = &scriptptr[-1] - apScript;

                g_skipBranch = false;

                C_GetNextVar();
                C_GetNextValue(LABEL_DEFINE);

                if (C_CheckMalformedBranch(lastScriptPtr))
                    continue;

               // scriptUpdateOpcodeForVariableType(ins);

                auto const offset = scriptptr - apScript;
                scriptptr++; //Leave a spot for the fail location

                C_ParseCommand(0);

                if (C_CheckEmptyBranch(tw, lastScriptPtr))
                    continue;

                auto const tempscrptr = apScript + offset;
                scriptWritePointer((intptr_t)scriptptr, tempscrptr);

                j = C_GetKeyword();

                if (j == concmd_else)
                    checking_ifelse++;

                continue;
            }


 
        case concmd_endevent:

            if (EDUKE32_PREDICT_FALSE(!g_scriptEventOffset))
            {
                C_ReportError(-1);
                Printf("%s:%d: error: found `endevent' without open `onevent'.\n",g_scriptFileName,line_number);
                errorcount++;
            }
            if (EDUKE32_PREDICT_FALSE(num_squigilly_brackets != 0))
            {
                C_ReportError(num_squigilly_brackets > 0 ? ERROR_OPENBRACKET : ERROR_CLOSEBRACKET);
                errorcount++;
            }

            g_scriptEventOffset = parsing_actor = 0;
            g_currentEvent = -1;
            Bsprintf(g_szCurrentBlockName,"(none)");
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
    Printf("%d/%d labels\n", labelcnt,
        (int32_t) min((MAXSECTORS * sizeof(sectortype)/sizeof(int32_t)),
            MAXSPRITES * sizeof(spritetype)/(1<<6)));

    int i, j;

    for (i=MAXTILES-1, j=0; i>=0; i--)
    {
        if (g_tile[i].execPtr)
            j++;
    }
    if (j) Printf("%d actors", j);

    Printf("\n");
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

    auto kFile = fileSystem.OpenFileReader(fileName);

	if (!kFile.isOpen())
    {
		if (!kFile.isOpen())
		{
			I_Error(tempbuf, "CON file `%s' missing.", fileName);
		}
	}

    int const kFileLen = kFile.GetLength();

    Printf("Compiling: %s (%d bytes)\n", fileName, kFileLen);

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

    labelcnt        = 0;
    g_defaultLabelCnt = 0;
    scriptptr       = apScript + 3;  // move permits constants 0 and 1; moveptr[1] would be script[2] (reachable?)
    warningcount      = 0;
    errorcount        = 0;
    line_number      = 1;
    g_totalLines      = 0;

    Bstrcpy(g_scriptFileName, fileName);

    C_ParseCommand(1);

    if (userConfig.AddCons) for (FString & m : *userConfig.AddCons.get())
    {
		C_Include(m);
    }
	userConfig.AddCons.reset();
    
    if (errorcount > 63)
        Printf("fatal error: too many errors: Aborted\n");

    //*script = (intptr_t) scriptptr;

    DO_FREE_AND_NULL(mptr);

    if (warningcount || errorcount)
        Printf("Found %d warning(s), %d error(s).\n", warningcount, errorcount);

    if (errorcount)
    {
        Bsprintf(buf, "Error compiling CON files.");
        G_GameExit(buf);
    }

    g_totalLines += line_number;

    C_SetScriptSize(scriptptr-apScript+8);

    Printf("Script compiled in %dms, %ld bytes%s\n", timerGetTicks() - startcompiletime,
                (unsigned long)(scriptptr-apScript), C_ScriptVersionString(g_scriptVersion));

    //freehashnames();
    freesoundhashnames();

    if (g_scriptDebug)
        C_PrintStats();

    C_InitQuotes();
}

void C_ReportError(int32_t iError)
{
    if (Bstrcmp(g_szCurrentBlockName,g_szLastBlockName))
    {
        if (g_scriptEventOffset || parsing_state || parsing_actor)
            Printf("%s: In %s `%s':\n",g_scriptFileName,g_scriptEventOffset?"event":parsing_actor?"actor":"state",g_szCurrentBlockName);
        else Printf("%s: At top level:\n",g_scriptFileName);
        Bstrcpy(g_szLastBlockName,g_szCurrentBlockName);
    }
    switch (iError)
    {
    case ERROR_CLOSEBRACKET:
        Printf("%s:%d: error: found more `}' than `{' before `%s'.\n",g_scriptFileName,line_number,tempbuf);
        break;
    case ERROR_EXCEEDSMAXTILES:
        Printf("%s:%d: error: `%s' value exceeds MAXTILES.  Maximum is %d.\n",g_scriptFileName,line_number,tempbuf,MAXTILES-1);
        break;
    case ERROR_EXPECTEDKEYWORD:
        Printf("%s:%d: error: expected a keyword but found `%s'.\n",g_scriptFileName,line_number,tempbuf);
        break;
    case ERROR_FOUNDWITHIN:
        Printf("%s:%d: error: found `%s' within %s.\n",g_scriptFileName,line_number,tempbuf,parsing_actor?"an actor":"a state");
        break;
    case ERROR_ISAKEYWORD:
        Printf("%s:%d: error: symbol `%s' is a keyword.\n",g_scriptFileName,line_number,label+(labelcnt<<6));
        break;
    case ERROR_OPENBRACKET:
        Printf("%s:%d: error: found more `{' than `}' before `%s'.\n",g_scriptFileName,line_number,tempbuf);
        break;
    case ERROR_NOTAGAMEVAR:
        Printf("%s:%d: error: symbol `%s' is not a variable.\n",g_scriptFileName,line_number,LAST_LABEL);
        break;
    case ERROR_PARAMUNDEFINED:
        Printf("%s:%d: error: parameter `%s' is undefined.\n",g_scriptFileName,line_number,tempbuf);
        break;
    case ERROR_SYNTAXERROR:
        Printf("%s:%d: error: syntax error.\n",g_scriptFileName,line_number);
        break;
    case ERROR_VARREADONLY:
        Printf("%s:%d: error: variable `%s' is read-only.\n",g_scriptFileName,line_number,LAST_LABEL);
        break;
    case ERROR_VARTYPEMISMATCH:
        Printf("%s:%d: error: variable `%s' is of the wrong type.\n",g_scriptFileName,line_number,LAST_LABEL);
        break;
    case WARNING_BADGAMEVAR:
        Printf("%s:%d: warning: variable `%s' should be either per-player OR per-actor, not both.\n",g_scriptFileName,line_number,LAST_LABEL);
        break;
    case WARNING_DUPLICATEDEFINITION:
        Printf("%s:%d: warning: duplicate definition `%s' ignored.\n",g_scriptFileName,line_number,LAST_LABEL);
        break;
    case WARNING_LABELSONLY:
        Printf("%s:%d: warning: expected a label, found a constant.\n",g_scriptFileName,line_number);
        break;
    case WARNING_VARMASKSKEYWORD:
        Printf("%s:%d: warning: variable `%s' masks keyword.\n", g_scriptFileName, line_number, LAST_LABEL);
        break;
    }
}

END_DUKE_NS
