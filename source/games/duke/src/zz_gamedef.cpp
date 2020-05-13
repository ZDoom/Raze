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
extern intptr_t parsing_event;
extern char *textptr;


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

int GetDefID(char const *label) { return hash_find(&h_gamevars, label); }

#define LAST_LABEL (label+(labelcnt<<6))
bool isaltok(const char c);

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


static int32_t C_GetNextKeyword(void) //Returns its code #
{
    int32_t i, l;

    C_SkipComments();

    if (*textptr == 0) // EOF
        return -1;

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


int parsecommand(int tw); // for now just run an externally parsed command.

int32_t C_ParseCommand(int32_t loop)
{
    int32_t j=0, k=0, tw;
    TArray<char> buffer;

    do
    {
        if (EDUKE32_PREDICT_FALSE(errorcount > 63 || (*textptr == '\0') || (*(textptr+1) == '\0') || C_SkipComments()))
            return 1;

        if (EDUKE32_PREDICT_FALSE(g_scriptDebug))
            C_ReportError(-1);

        g_lastKeyword = tw = C_GetNextKeyword();
            if (parsecommand(g_lastKeyword)) return 1;
            continue;
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


void C_Compile(const char *fileName)
{
    Bmemset(apScriptGameEvent, 0, sizeof(apScriptGameEvent));
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
}

void C_ReportError(int32_t iError)
{
    if (Bstrcmp(g_szCurrentBlockName,g_szLastBlockName))
    {
        if (parsing_event || parsing_state || parsing_actor)
            Printf("%s: In %s `%s':\n",g_scriptFileName,parsing_event?"event":parsing_actor?"actor":"state",g_szCurrentBlockName);
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
