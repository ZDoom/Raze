//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2020 - Christoph Oelckers

This file is part of Enhanced Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: This source file IS NOT USED in EDuke source.  It has been split
into many sub-files.

Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include <string.h>
#include "concmd.h"
#include "cmdlib.h"
#include "memarena.h"
#include "printf.h"
#include "filesystem.h"
#include "mapinfo.h"
#include "razemenu.h"
#include "global.h"
#include "m_argv.h"
#include "sounds.h"
#include "conlabel.h"
#include "conlabeldef.h"
#include "gi.h"

extern TArray<TPointer<MapRecord>> mapList;

BEGIN_DUKE_NS

enum { VERSIONCHECK = 41 };

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static TArray<FString> exclEpisodes;

void GameInterface::AddExcludedEpisode(const FString& episode)
{
	auto s = FStringTable::MakeMacro(episode.GetChars());
	s.StripRight();
	exclEpisodes.Push(s);
}


//---------------------------------------------------------------------------
//
// definitions needed by the parser.
//
//---------------------------------------------------------------------------

enum labeltypes {
	LABEL_ANY = -1,
	LABEL_DEFINE = 1,
	LABEL_STATE = 2,
	LABEL_ACTOR = 4,
	LABEL_ACTION = 8,
	LABEL_AI = 16,
	LABEL_MOVE = 32,
};

class labelstring
{
	char text[64];

public:
	char& operator[](size_t pos)
	{
		return text[pos];
	}
	operator const char* () { return text; }
	const char* GetChars() { return text; }
	int compare(const char* c) const { return strcmp(text, c); }
	int comparei(const char* c) const { return stricmp(text, c); }
	labelstring& operator=(const char* c) { strncpy(text, c, 64); text[63] = 0; return *this; }

};

struct TempMusic
{
	int volnum;
	int levlnum;
	FString music;
};

//---------------------------------------------------------------------------
//
// the actual parser
//
//---------------------------------------------------------------------------

class ConCompiler
{
	char* textptr = nullptr;
	int line_number = 0;
	int errorcount = 0, warningcount = 0;	// was named 'error' and 'warning' which is too generic for public variables and may clash with other code.
	int currentsourcefile = -1;
	unsigned parsing_actor = 0, parsing_event = 0;
	int parsing_state = 0;
	int num_squigilly_brackets = 0;
	int checking_ifelse = 0;
	int checking_switch = 0;
	labelstring parselabel= {};
	// This is for situations where the music gets defined before the map. Since the map records do not exist yet, we need a temporary buffer.
	TArray<TempMusic> tempMusic;
	char parsebuf[1024];
	TArray<char> parsebuffer; // global so that the storage is persistent across calls.
	int casecount = 0;
	int casescriptptr;


	void ReportError(int error);
	int getkeyword(const char* text);
	FString translatelabeltype(int type);
	bool ispecial(char c);
	void skiptoendofline();
	void skipwhitespace();
	void skipblockcomment();
	bool skipcomments();
	int keyword(void);
	void getlabel(void);
	void appendlabelvalue(labeltypes type, int value);
	void appendlabeladdress(labeltypes type, int offset = 0);
	int transword(void);
	int transnum(int type);
	void checkforkeyword();
	int parsecommand();

	// EDuke 2.x additions
	int CountCaseStatements();

public:
	void compilecon(const char* filenam);
	void setmusic();
	int getErrorCount() { return errorcount; }
};

//---------------------------------------------------------------------------
//
// label data
//
//---------------------------------------------------------------------------

static const char* labeltypenames[] = {
	"define",
	"state",
	"actor",
	"action",
	"ai",
	"move"
};

struct labeldef
{
	labelstring name;
	labeltypes type;
	int value;

	int compare(const char* c) const { return name.compare(c); }
	const char* GetChars() { return name.GetChars(); }

};

// These arrays contain the global output from the compiler.
static TArray<labeldef> labels;
TArray<int> ScriptCode;

//---------------------------------------------------------------------------
//
// synthesize the instruction list
//
//---------------------------------------------------------------------------

#define cmd(a) { #a, concmd_ ## a },
#define cmdx(a, b) { b, concmd_ ## a },
#define cmda(a,b) { #a, concmd_ ## b },

struct ConCommand
{
	char cmd[20];	// the longest instruction name is 'ifactornotstayput' at 17 characters so if this changes this field must be enlarged.
	int instr;
};

static ConCommand cmdList[] =
{
#include "condef.h"
};

#undef cmd
#undef cmdx
#undef cmda

static int cmdCmp(const void *a, const void *b)
{
	auto A = (ConCommand*)a;
	auto B = (ConCommand*)b;
	return strcmp(A->cmd, B->cmd);
}

void SortCommands()
{
	qsort(cmdList, countof(cmdList), sizeof(ConCommand), cmdCmp);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

enum
{
	ERROR_ISAKEYWORD = 1,
	ERROR_PARMUNDEFINED,
	WARNING_DUPLICATEDEFINITION,
	ERROR_COULDNOTFIND,
	ERROR_VARREADONLY,
	ERROR_NOTAGAMEDEF,
	ERROR_NOTAGAMEVAR,
	ERROR_OPENBRACKET,
	ERROR_CLOSEBRACKET,
	ERROR_NOENDSWITCH,
	ERROR_SYMBOLNOTRECOGNIZED
};

void ConCompiler::ReportError(int error)
{
	const char* fn = fileSystem.GetFileFullName(currentsourcefile);
	switch (error)
	{
	case ERROR_ISAKEYWORD:
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Symbol '%s' is a key word.\n",
			fn, line_number, parsebuf);
		break;
	case ERROR_PARMUNDEFINED:
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Parameter '%s' is undefined.\n",
			fn, line_number, parsebuf);
		break;
	case WARNING_DUPLICATEDEFINITION:
		Printf(TEXTCOLOR_YELLOW "  * WARNING.(%s, line %d) Duplicate definition '%s' ignored.\n",
			fn, line_number, parselabel.GetChars());
		break;
	case ERROR_COULDNOTFIND:
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Could not find '%s'.\n",
			fn, line_number, parselabel.GetChars());
		break;
	case ERROR_VARREADONLY:
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Variable '%s' is read-only.\n",
			fn, line_number, parselabel.GetChars());
		break;
	case ERROR_NOTAGAMEDEF:
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Symbol '%s' is not a Game Definition.\n",
			fn, line_number, parselabel.GetChars());
		break;
	case ERROR_NOTAGAMEVAR:
		Printf(TEXTCOLOR_RED "  * ERROR! (%s, line %d) Symbol '%s' is not a defined Game Variable.\n",
			fn, line_number, parselabel.GetChars());
		break;
	case ERROR_OPENBRACKET:
		Printf(TEXTCOLOR_RED "  * ERROR! (%s, line %d) Found more '{' than '}' before '%s'.\n",
			fn, line_number, parsebuf);
		break;
	case ERROR_CLOSEBRACKET:
		Printf(TEXTCOLOR_RED "  * ERROR! (%s, line %d) Found more '}' than '{' before '%s'.\n",
			fn, line_number, parsebuf);
		break;
	case ERROR_NOENDSWITCH:
		Printf(TEXTCOLOR_RED "  * ERROR! (%s, line %d) Did not find endswitch before '%s'.\n",
			fn, line_number, parsebuf);
		break;
	case ERROR_SYMBOLNOTRECOGNIZED:
		Printf(TEXTCOLOR_RED "  * ERROR! (%s, line %d) Symbol '%s' is not recognized.\n",
			fn, line_number, parsebuf);
		break;

	}
}

//---------------------------------------------------------------------------
//
// binary search for keyword
//
//---------------------------------------------------------------------------

int ConCompiler::getkeyword(const char* text)
{
	ptrdiff_t min = 0;
	ptrdiff_t max = countof(cmdList) - 1;

	while (min <= max)
	{
		auto mid = (min + max) >> 1;
		const int comp = strcmp(text, cmdList[mid].cmd);

		if (comp == 0)
		{
			return cmdList[mid].instr;
		}
		else if (comp > 0)
		{
			min = mid + 1;
		}
		else
		{
			max = mid - 1;
		}
	}
	return -1; 
}

//---------------------------------------------------------------------------
//
// label management
//
//---------------------------------------------------------------------------

FString ConCompiler::translatelabeltype(int type)
{
	FString buf;
	for (int i = 0; i < 6; i++) 
	{
		if (!(type & (1 << i))) continue;
		if (buf.Len()) buf += " or ";
		buf += labeltypenames[i];
	}
	return buf;
}

int findlabel(const char* text, bool ignorecase = false)
{
	for (unsigned j = 0; j < labels.Size(); j++)
	{
		if (labels[j].compare(text) == 0)
		{
			return j;
		}
	}
	if (ignorecase)
	{
		for (unsigned j = 0; j < labels.Size(); j++)
		{
			if (labels[j].name.comparei(text) == 0)
			{
				return j;
			}
		}
	}
	return -1;
}

// This is for the 'spawn' CCMD.
int getlabelvalue(const char* text)
{
	int lnum = findlabel(text, true);
	if (lnum >= 0 && labels[lnum].type != LABEL_DEFINE) return -1;
	return lnum < 0 ? -1 : labels[lnum].value;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ConCompiler::ispecial(char c)
{
	if (c == 0x0a)
	{
		line_number++;
		return true;
	}

	// oh joy - we need to skip some more characters here to allow running broken scripts.
	if (c == ' ' || c == '\t' || c == 0x0d || c == '(' || c == ')' || c == ',' || c == ';')
		return true;

	return false;
}

static bool isaltok(char c)
{
	// isalnum pukes on negative input.
	return c > 0 && (isalnum((uint8_t)c) || c == '{' || c == '}' || c == '/' || c == '*' || c == '-' || c == '_' || c == '.');
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ConCompiler::skiptoendofline()
{
	while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
		textptr++;
}

void ConCompiler::skipwhitespace()
{
	while (*textptr == ' ' || *textptr == '\t' || *textptr == '\r' || *textptr == '\n')
	{
		if (*textptr == '\n') line_number++;
		textptr++;
	}
}

void ConCompiler::skipblockcomment()
{
	while (*textptr != '*' || textptr[1] != '/')
	{
		if (*textptr == '\n') line_number++;
		if (*textptr == 0) return;	// reached the end of the file
		textptr++;
	}
	textptr += 2;
}

bool ConCompiler::skipcomments()
{
	while (true)
	{
		skipwhitespace();
		if (*textptr == '/' && textptr[1] == '/')
		{
			skiptoendofline();
			continue;
		}
		if (*textptr == '/' && textptr[1] == '*')
		{
			skipblockcomment();
			continue;
		}
		// stop if we got something else

		// this line got added to skip over a stray semicolon in RR's COOT.CON.
		if (ispecial(*textptr)) textptr++;
		break;
	}
	return *textptr != 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ConCompiler::keyword(void)
{
	int i;
	const char* temptextptr;

	skipcomments();
	temptextptr = textptr;

	while (isaltok(*temptextptr) == 0)
	{
		temptextptr++;
		if (*temptextptr == 0)
			return 0;
	}

	i = 0;
	while (isaltok(*temptextptr))
	{
		parsebuf[i] = *(temptextptr++);
		i++;
	}
	parsebuf[i] = 0;

	return getkeyword(parsebuf);
}
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int getlabeloffset(LABELS* pLabel, const char* psz)
{
	// find the label psz in the table pLabel.
	// returns the offset in the array for the label, or -1
	int i;

	for (i = 0; pLabel[i].lId >= 0; i++)
	{
		if (!stricmp(pLabel[i].name, psz))
		{
			//	printf("Label has flags of %02X\n",pLabel[i].flags);
			return i;
		}
	}
	return -1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ConCompiler::getlabel(void)
{
	int i;

	skipcomments();

	while (isalnum((uint8_t) * textptr & 0xff) == 0)
	{
		if (*textptr == 0x0a) line_number++;
		textptr++;
		if (*textptr == 0)
			return;
	}

	i = 0;
	while (ispecial(*textptr) == 0 && *textptr != ']')
		parselabel[i++] = *(textptr++);

	parselabel[i] = 0;
}

//---------------------------------------------------------------------------
//
// script buffer access wrappers.
//
//---------------------------------------------------------------------------

static void setscriptvalue(int offset, int value)
{
	ScriptCode[offset] = value;
}

static int scriptpos()
{
	return ScriptCode.Size();
}

static void appendscriptvalue(int value)
{
	ScriptCode.Push(value);
}

static int popscriptvalue()
{
	decltype(ScriptCode)::value_type p = 0;
	ScriptCode.Pop(p);
	return p;
}

void reservescriptspace(int space)
{
	for (int i = 0; i < space; i++) ScriptCode.Push(0);
}

/*
void pushlabeladdress()
{
	labelcode.Push(script.Size());
}
*/

void ConCompiler::appendlabelvalue(labeltypes type, int value)
{
	labels.Reserve(1);
	labels.Last().type = type;
	labels.Last().name = parselabel;
	labels.Last().value = value;
}

void ConCompiler::appendlabeladdress(labeltypes type, int offset)
{
	appendlabelvalue(type, ScriptCode.Size() + offset);
}


//---------------------------------------------------------------------------
//
//Returns its code #
//
//---------------------------------------------------------------------------

int ConCompiler::transword(void) 
{
	int i, l;

	skipcomments();

	while (isaltok(*textptr) == 0)
	{
		if (*textptr == 0x0a) line_number++;
		if (*textptr == 0)
			return -1;
		textptr++;
	}

	l = 0;
	while (isaltok(*(textptr + l)) && !(*(textptr + l) == '.'))
	{
		if (l < 1023)
		{
			parsebuf[l] = textptr[l];
		}
		l++;
	}
	parsebuf[l] = 0;

	i = getkeyword(parsebuf);
	if (i >= 0)
	{
		appendscriptvalue(i);
		textptr += l;
		return i;
	}

	textptr += l;

	const char* fn = fileSystem.GetFileFullName(currentsourcefile);
	if (parsebuf[0] == '{' && parsebuf[1] != 0)
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Expecting a SPACE or CR between '{' and '%s'.\n", fn, line_number, parsebuf + 1);
	else if (parsebuf[0] == '}' && parsebuf[1] != 0)
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Expecting a SPACE or CR between '}' and '%s'.\n", fn, line_number, parsebuf + 1);
	else if (parsebuf[0] == '/' && parsebuf[1] == '/' && parsebuf[2] != 0)
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Expecting a SPACE between '//' and '%s'.\n", fn, line_number, parsebuf + 2);
	else if (parsebuf[0] == '/' && parsebuf[1] == '*' && parsebuf[2] != 0)
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Expecting a SPACE between '/*' and '%s'.\n", fn, line_number, parsebuf + 2);
	else if (parsebuf[0] == '*' && parsebuf[1] == '/' && parsebuf[2] != 0)
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Expecting a SPACE between '*/' and '%s'.\n", fn, line_number, parsebuf + 2);
	else
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Keyword expected, got '%s'.\n", fn, line_number, parsebuf + 2);

	errorcount++;
	return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ConCompiler::transnum(int type)
{
	int l;

	while (isaltok(*textptr) == 0)
	{
		if (*textptr == 0x0a) line_number++;
		textptr++;
		if (*textptr == 0)
			return 0;
	}


	l = 0;
	while (isaltok(*(textptr + l)))
	{
		if (l < 1023)
		{
			parsebuf[l] = textptr[l];
		}
		l++;
	}
	parsebuf[l] = 0;

	if (getkeyword(parsebuf) >= 0)
	{
		errorcount++;
		ReportError(ERROR_ISAKEYWORD);
		textptr += l;
	}


	for (unsigned i = 0; i < labels.Size(); i++)
	{
		if (labels[i].compare(parsebuf) == 0)
		{
			// Non-values can be compared with 0.
			if (labels[i].type & type || (labels[i].value == 0))
			{
				appendscriptvalue(labels[i].value);
				textptr += l;
				return labels[i].value;
			}
			appendscriptvalue(0);
			textptr += l;
			auto el = translatelabeltype(type);
			auto gl = translatelabeltype(labels[i].type);
			const char* fn = fileSystem.GetFileFullName(currentsourcefile);
			Printf(TEXTCOLOR_YELLOW "  * WARNING.(%s, line %d) %s: Expected a '%s' label but found a '%s' label instead.\n", fn, line_number, labels[i].GetChars(), el.GetChars(), gl.GetChars());
			return -1;  // valid label name, but wrong type
		}
	}

	if (isdigit((uint8_t) *textptr) == 0 && *textptr != '-')
	{
		ReportError(ERROR_PARMUNDEFINED);
		errorcount++;
		textptr += l;
#ifdef FOR_LATER
		if (GetDefID(parsebuf) >= 0)
		{
			Printf(TEXTCOLOR_ORANGE "     Game Variable not expected\n");
		}
#endif
		return 0;
	}

	// Now it's getting nasty... With all of C's integer conversion functions we have to look for undefined behavior and truncation problems. This one's the least problematic approach
	// that ignores octal conversion.
	int64_t value;
	char *outp;
	bool ishex = (textptr[0] == '0' && tolower(textptr[1]) == 'x') || (textptr[0] == '-' && textptr[1] == '0' && tolower(textptr[2]) == 'x');
	if (*textptr == '-') value = strtoll(textptr, &outp, ishex? 0 : 10);
	else value = strtoull(textptr, &outp, ishex ? 0 : 10);
	if (*outp != 0)
	{
		// conversion was not successful.
	}
	appendscriptvalue(int(value));	// truncate the parsed value to 32 bit.

	if (type != LABEL_DEFINE && value != 0)
	{
		const char* fn = fileSystem.GetFileFullName(currentsourcefile);
		Printf(TEXTCOLOR_YELLOW "  * WARNING.(%s, line %d) Expected an identifier, got a numeric literal %d.\n", fn, line_number, (int)value);
	}


	textptr += l;
	return int(value);
}

//---------------------------------------------------------------------------
//
// just to reduce some excessive boilerplate. This block reappeared
// endlessly in parsecommand
//
//---------------------------------------------------------------------------

void ConCompiler::checkforkeyword()
{
	if (getkeyword(parselabel) >= 0)
	{
		errorcount++;
		ReportError(ERROR_ISAKEYWORD);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ConCompiler::CountCaseStatements()
{
	int lCount;
	char* temptextptr;
	int savescript;
	int savecase;
	int temp_line_number;


	temp_line_number = line_number;

	casecount = 0;
	temptextptr = textptr;
	savescript = scriptpos();
	savecase = casescriptptr;
	casescriptptr = 0;
	while (parsecommand() == 0)
	{
		;
	}
	// since we processed the endswitch, we need to re-increment checking_switch
	checking_switch++;

	textptr = temptextptr;

	ScriptCode.Resize(savescript);

	line_number = temp_line_number;

	lCount = casecount;
	casecount = 0;
	casescriptptr = savecase;
	return lCount;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ConCompiler::parsecommand()
{
	const char* fn = fileSystem.GetFileFullName(currentsourcefile);
	int i, j, k;
	int tempscrptr;
	uint8_t done, temp_ifelse_check;// , tw;
	int temp_line_number;
	int temp_current_file;
	int lnum;

	// Do not count warnings here and allow more errors before bailing out.
	if ((errorcount) > 64 || (*textptr == '\0') || (*(textptr + 1) == '\0')) return 1;
	int tw = transword();

	switch (tw)
	{
	default:
	case -1:
		return 0; //End

	case concmd_state:
		if (parsing_actor == 0 && parsing_state == 0)
		{
			getlabel();
			popscriptvalue();
			appendlabeladdress(LABEL_STATE);

			parsing_state = 1;

			return 0;
		}

		getlabel();
		checkforkeyword();

		lnum = findlabel(parselabel);

		if (lnum < 0)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) State '%s' not found.\n", fn, line_number, parselabel.GetChars());
			errorcount++;
			return 0;
		}
		appendscriptvalue(labels[lnum].value);
		return 0;

	case concmd_ends:
		if (parsing_state == 0)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found 'ends' with no 'state'.\n", fn, line_number);
			errorcount++;
		}
		//			  else
		{
			if (num_squigilly_brackets > 0)
			{
				ReportError(ERROR_OPENBRACKET);
				errorcount++;
			}
			if (num_squigilly_brackets < 0)
			{
				ReportError(ERROR_CLOSEBRACKET);
				errorcount++;
			}
			parsing_state = 0;
		}
		return 0;

	case concmd_gamevar:
	{
		// syntax: gamevar <var1> <initial value> <flags>
		// defines var1 and sets initial value.
		// flags are used to define usage
		// (see top of this files for flags)
		getlabel();	
		// Check to see it's already defined
		popscriptvalue();

		if (getkeyword(parselabel) >= 0)
		{
			errorcount++;
			ReportError(ERROR_ISAKEYWORD);
			return 0;
		}

		transnum(LABEL_DEFINE);	// get initial value
		j = popscriptvalue();

		transnum(LABEL_DEFINE);	// get flags
		lnum = popscriptvalue();
		if (strlen(parselabel) > (MAXVARLABEL - 1))
		{
			warningcount++;
			Printf(TEXTCOLOR_RED "  * WARNING.(%s, line %d) Variable Name '%s' too int (max is %d)\n", fn, line_number, parselabel.GetChars(), MAXVARLABEL - 1);
			return 0;
		}
		int res = AddGameVar(parselabel, j, lnum & (~(GAMEVAR_FLAG_DEFAULT | GAMEVAR_FLAG_SECRET)));
		if (res < 0)
		{
			errorcount++;
			if (res == -1) Printf(TEXTCOLOR_RED "  * ERROR.(%s, line %d) Duplicate game variable definition '%s'.\n", fn, line_number, parselabel.GetChars());
			else if (res == -2) Printf(TEXTCOLOR_RED "  * ERROR.(%s, line %d) '%s' maximum number of game variables exceeded.\n", fn, line_number, parselabel.GetChars());
			return 0;
		}

		return 0;
	}
	case concmd_define:
		getlabel();
		checkforkeyword();
		lnum = findlabel(parselabel);
		if (lnum >= 0)
		{
			warningcount++;
			ReportError(WARNING_DUPLICATEDEFINITION);
			break;
		}

		transnum(LABEL_DEFINE);
		i = popscriptvalue();
		if (lnum < 0)
		{
			appendlabelvalue(LABEL_DEFINE, i);
		}
		popscriptvalue();
		return 0;

	case concmd_palfrom:

		for (j = 0; j < 4; j++)
		{
			if (keyword() == -1)
				transnum(LABEL_DEFINE);
			else break;
		}

		while (j < 4)
		{
			appendscriptvalue(0);
			j++;
		}
		return 0;

	case concmd_move:
		if (parsing_actor || parsing_state)
		{
			transnum(LABEL_MOVE);

			j = 0;
			while (keyword() == -1)
			{
				transnum(LABEL_DEFINE);
				j |= popscriptvalue();
			}
			appendscriptvalue(j);
		}
		else
		{
			popscriptvalue();
			getlabel();
			// Check to see it's already defined

			checkforkeyword();

			for (i = 0; i < (int)labels.Size(); i++)
				if (labels[i].compare(parselabel) == 0)
				{
					warningcount++;
					Printf(TEXTCOLOR_RED "  * WARNING.(%s, line %d) Duplicate move '%s' ignored.\n", fn, line_number, parselabel.GetChars());
					break;
				}
			if (i == (int)labels.Size())
				appendlabeladdress(LABEL_MOVE);
			for (j = 0; j < 2; j++)
			{
				if (keyword() >= 0) break;
				transnum(LABEL_DEFINE);
			}
			for (k = j; k < 2; k++)
			{
				appendscriptvalue(0);
			}
		}
		return 0;

	case concmd_music:
	{
		popscriptvalue();
		transnum(LABEL_DEFINE); // Volume Number (0/4)
		k = popscriptvalue() - 1;
		if (k < 0) specialmusic.Clear();

		i = 0;
		// get the file name...
		while (keyword() == -1)
		{
			while (isaltok(*textptr) == 0)
			{
				if (*textptr == 0x0a) line_number++;
				textptr++;
				if (*textptr == 0) break;
			}
			j = 0;
			parsebuffer.Clear();
			while (isaltok(*(textptr + j)))
			{
				parsebuffer.Push(textptr[j]);
				j++;
			}
			parsebuffer.Push(0);
			if (k >= 0)
			{
				tempMusic.Reserve(1);
				tempMusic.Last().volnum = k + 1;
				tempMusic.Last().levlnum = i + 1;
				tempMusic.Last().music = parsebuffer.Data();
			}
			else
			{
				specialmusic.Push(parsebuffer.Data());
			}
			textptr += j;
			i++;
		}

		return 0;
	}
	case concmd_include:
	{
		popscriptvalue();
		skipcomments();

		parsebuffer.Clear();
		while (isaltok(*textptr) == 0)
		{
			if (*textptr == 0x0a) line_number++;
			textptr++;
			if (*textptr == 0) break;
		}
		j = 0;
		while (isaltok(*textptr))
		{
			parsebuffer.Push(*(textptr++));
			j++;
		}
		parsebuffer.Push(0);

		auto fni = fileSystem.FindFile(parsebuffer.Data());
		if (fni < 0)
		{
			errorcount++;
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Could not find '%s'.\n", fn, line_number, parsebuffer.Data());

			ReportError(ERROR_COULDNOTFIND);
			return 0;
		}

		auto data = fileSystem.GetFileData(fni, 1);

		temp_current_file = currentsourcefile;
		currentsourcefile = fni;

		temp_line_number = line_number;
		line_number = 1;
		temp_ifelse_check = checking_ifelse;
		checking_ifelse = 0;
		auto origtptr = textptr;
		textptr = (char*)data.Data();

		do
			done = parsecommand();
		while (done == 0);

		textptr = origtptr;
		line_number = temp_line_number;
		checking_ifelse = temp_ifelse_check;
		currentsourcefile = temp_current_file;
		if (*textptr == '"') textptr++;	// needed for RR.

		return 0;
	}

	case concmd_ai:
		if (parsing_actor || parsing_state)
			transnum(LABEL_AI);
		else
		{
			popscriptvalue();
			getlabel();
			checkforkeyword();

			lnum = findlabel(parselabel);
			if (lnum >= 0)
			{
				warningcount++;
				Printf(TEXTCOLOR_RED "  * WARNING.(%s, line %d) Duplicate ai '%s' ignored.\n", fn, line_number, parselabel.GetChars());
			}
			else appendlabeladdress(LABEL_AI);

			for (j = 0; j < 3; j++)
			{
				if (keyword() >= 0) break;
				if (j == 2)
				{
					k = 0;
					while (keyword() == -1)
					{
						transnum(LABEL_DEFINE);
						k |= popscriptvalue();
					}
					appendscriptvalue(k);
					return 0;
				}
				else transnum(j==0? LABEL_ACTION : LABEL_MOVE);
			}
			for (k = j; k < 3; k++)
			{
				appendscriptvalue(0);
			}
		}
		return 0;

	case concmd_action:
		if (parsing_actor || parsing_state)
			transnum(LABEL_ACTION);
		else
		{
			popscriptvalue();
			getlabel();
			checkforkeyword();

			lnum = findlabel(parselabel);
			if (lnum >= 0)
			{
				warningcount++;
				Printf(TEXTCOLOR_RED "  * WARNING.(%s, line %d) Duplicate event '%s' ignored.\n", fn, line_number, parselabel.GetChars());
			}
			else appendlabeladdress(LABEL_ACTION);

			for (j = 0; j < 5; j++)
			{
				if (keyword() >= 0) break;
				transnum(LABEL_DEFINE);
			}
			for (k = j; k < 5; k++)
			{
				appendscriptvalue(0);
			}
		}
		return 0;

	case concmd_actor:
	case concmd_useractor:	// merged with 'actor' because the code is identical except for the added type parameter.
	{
		if (parsing_state)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found 'actor' within 'state'.\n", fn, line_number);
			errorcount++;
		}

		if (parsing_actor)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found 'actor' within 'actor'.\n", fn, line_number);
			errorcount++;
		}

		num_squigilly_brackets = 0;
		popscriptvalue();
		parsing_actor = scriptpos();

		j = 0;
		if (tw == concmd_useractor)
		{ 
			transnum(LABEL_DEFINE);
			j = popscriptvalue();
		}

		transnum(LABEL_DEFINE);
		lnum = popscriptvalue();

		gs.actorinfo[lnum].scriptaddress = parsing_actor;	// TRANSITIONAL should only store an index
		if (tw == concmd_useractor)
		{
			if (j & 1)
				gs.actorinfo[lnum].flags |= SFLAG_BADGUY;

			if (j & 2)
				gs.actorinfo[lnum].flags |= (SFLAG_BADGUY | SFLAG_BADGUYSTAYPUT);
		}

		for (j = 0; j < 4; j++)
		{
			if (j == 3)
			{
				j = 0;
				while (keyword() == -1)
				{
					transnum(LABEL_DEFINE);

					j |= popscriptvalue();
				}
				appendscriptvalue(j);
				break;
			}
			else
			{
				if (keyword() >= 0)
				{
					reservescriptspace(4 - j);
					break;
				}
				switch (j)
				{
				case 0: transnum(LABEL_DEFINE); break;
				case 1: transnum(LABEL_ACTION); break;
				case 2: transnum(LABEL_MOVE | LABEL_DEFINE); break;
				}
				// This code was originally here but is a no-op, because both source and destination are the same here.			
				//*(parsing_actor + j) = *(scriptaddress - 1);
			}
		}

		checking_ifelse = 0;

		return 0;
		}

	case concmd_onevent:
		if (parsing_state)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found 'onevent' within 'state'.\n", fn, line_number);
			errorcount++;
		}

		if (parsing_actor)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found 'onevent' within 'actor'.\n", fn, line_number);
			errorcount++;
		}

		num_squigilly_brackets = 0;
		popscriptvalue();
		parsing_event = parsing_actor = scriptpos();

		transnum(LABEL_DEFINE);
		j = popscriptvalue();
		if (j< 0 || j> EVENT_MAXEVENT)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Invalid Event ID.\n", fn, line_number);
			errorcount++;
			return 0;
		}
		apScriptGameEvent[j] = parsing_event;

		checking_ifelse = 0;

		return 0;


	case concmd_cstat:
		transnum(LABEL_DEFINE);
#if 0
		// the following checks are being performed by EDuke32 and RedNukem - not sure if this really should be done.
		// DukeGDX and RedneckGDX do not perform these checks. Code pasted here for making a decision later.

		i = popscriptvalue();
		if (i == 32767)
		{
			i = 32768;
			Printf(TEXTCOLOR_RED "  * WARNING!(%s, line %d) tried to set cstat 32767, using 32768 instead.\n", fn, line_number);
			warningcount++;
		}
		appendscriptvalue(i);
#endif
		return 0;


	case concmd_sound:
	case concmd_globalsound:
	case concmd_soundonce:
	case concmd_stopsound:
	case concmd_lotsofglass:
	case concmd_strength:
	case concmd_shoot:
	case concmd_addphealth:
	case concmd_spawn:
	case concmd_count:
	case concmd_endofgame:
	case concmd_endoflevel:
	case concmd_spritepal:
	case concmd_cactor:
	case concmd_quote:
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
	case concmd_espawn:
		transnum(LABEL_DEFINE);
		return 0;

	case concmd_addammo:
	case concmd_addweapon:
	case concmd_sizeto:
	case concmd_sizeat:
	case concmd_debris:
	case concmd_addinventory:
	case concmd_guts:
		transnum(LABEL_DEFINE);
		transnum(LABEL_DEFINE);
		return 0;

	case concmd_hitradius:
		transnum(LABEL_DEFINE);
		transnum(LABEL_DEFINE);
		transnum(LABEL_DEFINE);
		transnum(LABEL_DEFINE);
		transnum(LABEL_DEFINE);
		break;

	case concmd_else:
		if (checking_ifelse)
		{
			checking_ifelse--;
			tempscrptr = scriptpos();
			reservescriptspace(1); //Leave a spot for the fail location
			parsecommand();
			setscriptvalue(tempscrptr, scriptpos());
		}
		else
		{
			popscriptvalue();
			warningcount++;
			Printf(TEXTCOLOR_YELLOW "  * WARNING.(%s, line %d) Found 'else' with no 'if', ignored.\n", fn, line_number);
		}

		return 0;
	case concmd_setvar:
	case concmd_addvar:
	case concmd_subvar:
	case concmd_randvar:
	case concmd_mulvar:
	case concmd_divvar:
	case concmd_modvar:
	case concmd_andvar:
	case concmd_orvar:
	case concmd_xorvar:
		// syntax: [rand|add|set]var	<var1> <const1>
		// sets var1 to const1
		// adds const1 to var1 (const1 can be negative...)

		// get the ID of the DEF
		getlabel();	

		// Check to see if it's a keyword
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		if (aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
		{
			errorcount++;
			ReportError(ERROR_VARREADONLY);
			return 0;

		}
		appendscriptvalue(i);	// the ID of the DEF (offset into array...)

		transnum(LABEL_DEFINE);	// the number to check against...
		return 0;

	case concmd_setvarvar:
	case concmd_addvarvar:
	case concmd_subvarvar:
	case concmd_mulvarvar:
	case concmd_divvarvar:
	case concmd_modvarvar:
	case concmd_andvarvar:
	case concmd_orvarvar:
	case concmd_xorvarvar:
	case concmd_randvarvar:
	case concmd_sin:
	case concmd_gmaxammo:
	case concmd_smaxammo:
		// syntax: [add|set]varvar <var1> <var2>
		// sets var1 = var2
		// adds var1 and var2 with result in var1

		// get the ID of the DEF
		getlabel();	

		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		if (aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
		{
			errorcount++;
			ReportError(ERROR_VARREADONLY);
			return 0;

		}
		appendscriptvalue(i);	// the ID of the DEF (offset into array...)

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}

		appendscriptvalue(i);	// the ID of the DEF (offset into array...)
		return 0;

	case concmd_ifvarvarg:
	case concmd_ifvarvarl:
	case concmd_ifvarvare:
	case concmd_ifvarvarn:
	case concmd_ifvarvarand:

		// get the ID of the DEF
		getlabel();	

		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		appendscriptvalue(i);	// the ID of the DEF (offset into array...)

		// get the ID of the DEF
		getlabel();	

		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		appendscriptvalue(i);	// the ID of the DEF (offset into array...)
		goto if_common;

	case concmd_ifvarl:
	case concmd_ifvarg:
	case concmd_ifvare:
	case concmd_ifvarn:
	case concmd_ifvarand:

		// get the ID of the DEF
		getlabel();	

		checkforkeyword();
		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEVAR);
			return 0;
		}
		appendscriptvalue(i);	// the ID of the DEF (offset into array...)

		transnum(LABEL_DEFINE);	// the number to check against...
		goto if_common;

	case concmd_addlogvar:
		// syntax: addlogvar <var>

		appendscriptvalue(currentsourcefile);
		appendscriptvalue(line_number);

		// get the ID of the DEF
		getlabel();	

		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		appendscriptvalue(i);

		return 0;

	case concmd_addlog:
		// syntax: addlog

		// source file.
		appendscriptvalue(currentsourcefile);

		// prints the line number in the log file.
		appendscriptvalue(line_number);
		return 0;

	case concmd_ifp:
		j = 0;
		do
		{
			transnum(LABEL_DEFINE);
			j |= popscriptvalue();
		} while (keyword() == -1);
		appendscriptvalue(j);
		goto if_common;

	case concmd_ifsound:
		transnum(LABEL_DEFINE);
		goto if_common;

	case concmd_ifpinventory:
		transnum(LABEL_DEFINE);
		[[fallthrough]];
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
		//		  case 74:
	case concmd_ifphealthl:
	case concmd_ifspritepal:
	case concmd_ifgotweaponce:
	case concmd_ifangdiffl:
	case concmd_ifactorhealthg:
	case concmd_ifactorhealthl:
	case concmd_ifsoundid:
	case concmd_ifsounddist:
	case concmd_ifplayersl:
		transnum(tw == concmd_ifai? LABEL_AI : tw == concmd_ifaction? LABEL_ACTION : tw == concmd_ifmove? LABEL_MOVE : LABEL_DEFINE);
		[[fallthrough]];
	case concmd_ifonwater:
	case concmd_ifinwater:
	case concmd_ifactornotstayput:
	case concmd_ifcansee:
	case concmd_ifhitweapon:
	case concmd_ifsquished:
	case concmd_ifdead:
	case concmd_ifcanshoottarget:
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
	// case concmd_iffindnewspot:	// RRDH
	// case concmd_ifpupwind:

	if_common:	// this code is identical for all 'if...'instructions.
	{
		tempscrptr = scriptpos();
		reservescriptspace(1); //Leave a spot for the fail location

		skipcomments();
		parsecommand();

		setscriptvalue(tempscrptr, scriptpos());
		auto kw = keyword();
		// Cannot be done - the code starts misbehaving with this check, it is especially noticeable on the soldiers in NAM.
		// Unfortunately this means one less error check, but ultimately CON is too broken to begin with anyway
#if 0
		if (k == concmd_else)	/ only increment checking_ifelse if there actually is an else. Otherwise this would break the entire checking logic and render it non-functional
#endif
			checking_ifelse++;
		return 0;
	}
	case concmd_leftbrace:
		num_squigilly_brackets++;
		do
			done = parsecommand();
		while (done == 0);
		return 0;
	case concmd_rightbrace:
		num_squigilly_brackets--;
		if (num_squigilly_brackets < 0)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found more '}' than '{'.\n", fn, line_number);
			errorcount++;
		}
		return 1;
	case concmd_betaname:
		popscriptvalue();
		// not used anywhere, just parse over it.
		skiptoendofline();
		return 0;

	case concmd_definevolumename:
	{
		popscriptvalue();
		transnum(LABEL_DEFINE);
		j = popscriptvalue();
		while (*textptr == ' ' || *textptr == '\t') textptr++;

		i = 0;

		parsebuffer.Clear();
		while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)		// JBF 20040127: end of file checked
		{
			parsebuffer.Push(*textptr);
			textptr++, i++;
		}
		parsebuffer.Push(0);
		// We need both a volume and a cluster for this new episode.
		auto vol = MustFindVolume(j);
		auto clust = MustFindCluster(j + 1);
		FString s = FStringTable::MakeMacro(parsebuffer.Data(), i);;
		s.StripRight();
		vol->name = clust->name = s;
		if (j > 0) vol->flags |= VF_SHAREWARELOCK;
		if (exclEpisodes.Size())
		{
			for (auto& episode : exclEpisodes)
			{
				if (vol->name == episode)
				{
					vol->flags |= VF_HIDEFROMSP;
				}
			}
		}
		return 0;
	}
	case concmd_defineskillname:
	{
		popscriptvalue();
		transnum(LABEL_DEFINE);
		j = popscriptvalue();
		while (*textptr == ' ' || *textptr == '\t') textptr++;

		i = 0;

		parsebuffer.Clear();
		while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)		// JBF 20040127: end of file checked
		{
			parsebuffer.Push(*textptr);
			textptr++, i++;
		}
		parsebuffer.Push(0);
		FString s = FStringTable::MakeMacro(parsebuffer.Data(), i);
		s.StripRight();
		gSkillNames[j] = s;
		return 0;
	}
	case concmd_definelevelname:
	{
		popscriptvalue();
		transnum(LABEL_DEFINE);
		j = popscriptvalue();
		transnum(LABEL_DEFINE);
		k = popscriptvalue();
		while (*textptr == ' ' || *textptr == '\t') textptr++;

		i = 0;
		parsebuffer.Clear();
		while (*textptr != ' ' && *textptr != '\t' && *textptr != 0x0a && *textptr != 0x0d && *textptr != 0)	// JBF 20040127: end of file checked
		{
			parsebuffer.Push(*textptr);
			textptr++, i++;
		}
		parsebuffer.Push(0);
		auto map = FindMapByIndexOnly(j + 1, k + 1);
		if (!map) map = AllocateMap();
		map->SetFileName(parsebuffer.Data());
		if (k == 0)
		{
			auto vol = MustFindVolume(j);
			vol->startmap = map->labelName;
		}

		while (*textptr == ' ' || *textptr == '\t') textptr++;

		map->parTime =
			(((*(textptr + 0) - '0') * 10 + (*(textptr + 1) - '0')) * 60) +
			(((*(textptr + 3) - '0') * 10 + (*(textptr + 4) - '0')));

		textptr += 5;
		while (*textptr == ' ' || *textptr == '\t') textptr++;

		map->designerTime =
			(((*(textptr + 0) - '0') * 10 + (*(textptr + 1) - '0')) * 60) +
			(((*(textptr + 3) - '0') * 10 + (*(textptr + 4) - '0')));

		SetLevelNum(map, makelevelnum(j + 1, k + 1));

		map->cluster = j + 1;

		textptr += 5;
		while (*textptr == ' ' || *textptr == '\t') textptr++;

		i = 0;

		parsebuffer.Clear();
		while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)		// JBF 20040127: end of file checked
		{
			parsebuffer.Push(*textptr);
			textptr++, i++;
		}
		parsebuffer.Push(0);
		map->name = FStringTable::MakeMacro(parsebuffer.Data());
		map->name.StripRight();
		return 0;
	}
	case concmd_definequote:
		popscriptvalue();
		transnum(LABEL_DEFINE);
		k = popscriptvalue();
		if (k >= MAXQUOTES)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Quote number exceeds limit of %d.\n", fn, line_number, MAXQUOTES);
			errorcount++;
		}

		i = 0;
		while (*textptr == ' ' || *textptr == '\t') textptr++;

		parsebuffer.Clear();
		while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)		// JBF 20040127: end of file checked
		{
			parsebuffer.Push(*textptr);
			textptr++, i++;
		}
		parsebuffer.Push(0);
		quoteMgr.InitializeQuote(k, parsebuffer.Data(), true);
		return 0;
	case concmd_definesound:
	{
		popscriptvalue();
		transnum(LABEL_DEFINE);
		k = popscriptvalue();
		i = 0;
		while (*textptr == ' ' || *textptr == '\t') textptr++;

		parsebuffer.Clear();
		while (*textptr != ' ' && *textptr != '\t' && *textptr != 0)		// JBF 20040127: end of file checked
		{
			parsebuffer.Push(*textptr);
			textptr++, i++;
		}
		parsebuffer.Push(0);

		transnum(LABEL_DEFINE);
		int ps = popscriptvalue();
		transnum(LABEL_DEFINE);
		int pe = popscriptvalue();
		transnum(LABEL_DEFINE);
		int pr = popscriptvalue();
		transnum(LABEL_DEFINE);
		int m = popscriptvalue();
		transnum(LABEL_DEFINE);
		int vo = popscriptvalue();
		S_DefineSound(k, parsebuffer.Data(), ps, pe, pr, m, vo, 1.f);
		return 0;
	}

	case concmd_endevent:
		if (parsing_event == 0)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found 'endevent' without defining 'onevent'.\n", fn, line_number);
			errorcount++;
		}
		//			  else
		{
			if (num_squigilly_brackets > 0)
			{
				Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found more '{' than '}' before 'endevent'.\n", fn, line_number);
				errorcount++;
			}
			parsing_event = 0;
			parsing_actor = 0;
		}

		return 0;

	case concmd_enda:
		if (parsing_actor == 0)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found 'enda' without defining 'actor'.\n", fn, line_number);
			errorcount++;
		}
		//			  else
		{
			if (num_squigilly_brackets > 0)
			{
				Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found more '{' than '}' before 'enda'.\n", fn, line_number);
				errorcount++;
			}
			parsing_actor = 0;
		}

		return 0;
	case concmd_break:
		if (checking_switch)
		{
			return 1;
		}
		[[fallthrough]];
	case concmd_fall:
	case concmd_tip:
		//		  case 21:
	case concmd_killit:	//KILLIT
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
	case concmd_nullop:
	case concmd_destroyit:
	case concmd_larrybird:
	case concmd_strafeleft:
	case concmd_straferight:
	case concmd_slapplayer:
		//case 122:
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
		//case concmd_leavetrax:		// RRDH
		//case concmd_leavedroppings:
		//case concmd_deploybias:
		return 0;
	case concmd_gamestartup:
	{
		// What a mess. The only way to detect which game version we are running is to count the parsed values here.
		int params[34]; // 34 is the maximum for RRRA.
		int pcount = 0;
		for (int ii = 0; ii < 34; ii++)
		{
			transnum(LABEL_DEFINE);
			params[pcount++] = popscriptvalue();
			if (keyword() != -1) break;
		}
		int pget = 0;

		if (!isRR())
		{
			if (pcount == 30) g_gameType |= GAMEFLAG_PLUTOPAK;
			else if (pcount == 31) g_gameType |= GAMEFLAG_PLUTOPAK | GAMEFLAG_WORLDTOUR;
			else if (pcount == 22) g_gameType |= GAMEFLAG_SHAREWARE;
			else if (pcount != 26) I_FatalError("Invalid CONs. Cannot detect version. gamestartup has %d entries", pcount);
		}
		gameinfo.gametype = g_gameType;

		popscriptvalue();
		auto parseone = [&]() { return params[pget++]; };

		ud.const_visibility = parseone();
		gs.impact_damage = parseone();
		gs.max_player_health = parseone();
		gs.max_armour_amount = parseone();
		gs.respawnactortime = parseone();
		gs.respawnitemtime = parseone();
		gs.playerfriction = parseone();
		if (isPlutoPak() || isRR()) gs.gravity = parseone();
		gs.rpgblastradius = parseone();
		gs.pipebombblastradius = parseone();
		gs.shrinkerblastradius = parseone();
		gs.tripbombblastradius = parseone();
		gs.morterblastradius = parseone();
		gs.bouncemineblastradius = parseone();
		gs.seenineblastradius = parseone();

		gs.max_ammo_amount[1] = parseone();
		gs.max_ammo_amount[2] = parseone();
		gs.max_ammo_amount[3] = parseone();
		gs.max_ammo_amount[4] = parseone();
		gs.max_ammo_amount[5] = parseone();
		gs.max_ammo_amount[6] = parseone();
		gs.max_ammo_amount[7] = parseone();
		gs.max_ammo_amount[8] = parseone();
		gs.max_ammo_amount[9] = parseone();
		if (isPlutoPak() || isRR()) gs.max_ammo_amount[11] = parseone();
		if (isRR()) gs.max_ammo_amount[12] = parseone();
		gs.camerashitable = parseone();
		gs.numfreezebounces = parseone();
		gs.freezerhurtowner = parseone();
		if (isPlutoPak() || isRR())
		{
			spriteqamount = clamp(parseone(), 0, 1024);
			gs.lasermode = parseone();
		}
		if (isWorldTour()) gs.max_ammo_amount[12] = parseone();
		if (isRRRA())
		{
			gs.max_ammo_amount[13] = parseone();
			gs.max_ammo_amount[14] = parseone();
			gs.max_ammo_amount[16] = parseone();
		}
		return 0;
	}

	case concmd_eventloadactor:
	{
		if (parsing_state)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found 'eventloadactor' within 'state'.\n", fn, line_number);
			errorcount++;
		}

		if (parsing_actor)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found 'eventloadactor' within 'actor'.\n", fn, line_number);
			errorcount++;
		}

		num_squigilly_brackets = 0;
		popscriptvalue();
		parsing_actor = scriptpos();

		transnum(LABEL_DEFINE);
		int n = popscriptvalue();
		gs.tileinfo[n].loadeventscriptptr = parsing_actor;
		checking_ifelse = 0;
		return 0;
	}
	case concmd_setsector:
	case concmd_getsector:
	{
		int lLabelID;
		// syntax getsector[<var>].x <VAR>
		// gets the value of sector [<var>].xxx into <VAR>

		// now get name of .xxx
		while ((*textptr != '['))
		{
			textptr++;
		}
		if (*textptr == '[')
			textptr++;

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		appendscriptvalue(i);

		// now get name of .xxx
		while (*textptr != '.')
		{
			if (*textptr == 0xa)
				break;
			if (!*textptr)
				break;

			textptr++;
		}
		if (*textptr != '.')
		{
			errorcount++;
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Syntax error.\n", fn, line_number);
			return 0;

		}
		textptr++;
		/// now pointing at 'xxx'
		getlabel();

		lLabelID = getlabeloffset(sectorlabels, parselabel);

		if (lLabelID == -1)
		{
			errorcount++;
			ReportError(ERROR_SYMBOLNOTRECOGNIZED);
			return 0;
		}
		appendscriptvalue(sectorlabels[lLabelID].lId);
		if (sectorlabels[lLabelID].flags & LABEL_HASPARM2)
		{
			getlabel();

			checkforkeyword();

			i = GetDefID(parselabel);
			if (i < 0)
			{	// not a defined DEF
				errorcount++;
				ReportError(ERROR_NOTAGAMEDEF);
				return 0;
			}
			appendscriptvalue(i);
		}
		else appendscriptvalue(0);

		// now at target VAR...

		// get the ID of the DEF
		getlabel();	

		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		if (tw == concmd_getsector && aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
		{
			errorcount++;
			ReportError(ERROR_VARREADONLY);
			return 0;
		}
		appendscriptvalue(i);

		break;
	}
	case concmd_findnearactor:
	{
		// syntax findnearactor <type> <maxdist> <getvar>
		// gets the sprite ID of the nearest actor within max dist
		// that is of <type> into <getvar>
		// -1 for none found

		transnum(LABEL_DEFINE);	// get <type>

		transnum(LABEL_DEFINE); // get maxdist

		getlabel();	

		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		if (aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
		{
			errorcount++;
			ReportError(ERROR_VARREADONLY);
			return 0;
		}
		appendscriptvalue(i);

		break;
	}
	case concmd_findnearactorvar:
	{
		// syntax findnearactorvar <type> <maxdistvar> <getvar>
		// gets the sprite ID of the nearest actor within max dist
		// that is of <type> into <getvar>
		// -1 for none found

		transnum(LABEL_DEFINE);	// get <type>
		getlabel();	

		checkforkeyword();
		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}

		// target var
		getlabel();	

		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		if (aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
		{
			errorcount++;
			ReportError(ERROR_VARREADONLY);
			return 0;

		}
		appendscriptvalue(i);
		break;
	}
	case concmd_sqrt:
	{
		// syntax sqrt <invar> <outvar>
		// gets the sqrt of invar into outvar

		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		appendscriptvalue(i);

		// target var
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		if (aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
		{
			errorcount++;
			ReportError(ERROR_VARREADONLY);
			return 0;

		}
		appendscriptvalue(i);

		break;
	}
	case concmd_setwall:
	case concmd_getwall:
	{
		int lLabelID;
		// syntax getwall[<var>].x <VAR>
		// gets the value of wall [<var>].xxx into <VAR>

		// now get name of .xxx
		while ((*textptr != '['))
		{
			textptr++;
		}
		if (*textptr == '[')
			textptr++;

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		appendscriptvalue(i);

		// now get name of .xxx
		while (*textptr != '.')
		{
			if (*textptr == 0xa)
				break;
			if (!*textptr)
				break;

			textptr++;
		}
		if (*textptr != '.')
		{
			errorcount++;
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Syntax error.\n", fn, line_number);
			return 0;

		}
		textptr++;
		/// now pointing at 'xxx'
		getlabel();

		lLabelID = getlabeloffset(walllabels, parselabel);

		if (lLabelID == -1)
		{
			errorcount++;
			ReportError(ERROR_SYMBOLNOTRECOGNIZED);
			return 0;
		}
		appendscriptvalue(walllabels[lLabelID].lId);

		if (walllabels[lLabelID].flags & LABEL_HASPARM2)
		{
			// get parm2
		// get the ID of the DEF
			getlabel();	
			checkforkeyword();

			i = GetDefID(parselabel);
			if (i < 0)
			{	// not a defined DEF
				errorcount++;
				ReportError(ERROR_NOTAGAMEDEF);
				return 0;
			}
			appendscriptvalue(i);	// the ID of the DEF (offset into array...)
		}
		else appendscriptvalue(0);

		// now at target VAR...

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		if (tw == concmd_getwall && aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
		{
			errorcount++;
			ReportError(ERROR_VARREADONLY);
			return 0;

		}
		appendscriptvalue(i);

		break;
	}
	case concmd_setplayer:
	case concmd_getplayer:
	{
		int lLabelID;
		// syntax getwall[<var>].x <VAR>
		// gets the value of wall [<var>].xxx into <VAR>

		// now get name of .xxx
		while ((*textptr != '['))
		{
			textptr++;
		}
		if (*textptr == '[')
			textptr++;

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		appendscriptvalue(i);

		// now get name of .xxx
		while (*textptr != '.')
		{
			if (*textptr == 0xa)
				break;
			if (!*textptr)
				break;

			textptr++;
		}
		if (*textptr != '.')
		{
			errorcount++;
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Syntax error.\n", fn, line_number);
			return 0;

		}
		textptr++;
		/// now pointing at 'xxx'
		getlabel();

		lLabelID = getlabeloffset(playerlabels, parselabel);
		if (lLabelID == -1)
		{
			errorcount++;
			ReportError(ERROR_SYMBOLNOTRECOGNIZED);
			return 0;
		}

		appendscriptvalue(playerlabels[lLabelID].lId);

		if (playerlabels[lLabelID].flags & LABEL_HASPARM2)
		{
			getlabel();	
			checkforkeyword();

			i = GetDefID(parselabel);
			if (i < 0)
			{	// not a defined DEF
				errorcount++;
				ReportError(ERROR_NOTAGAMEDEF);
				return 0;
			}
			appendscriptvalue(i);	// the ID of the DEF (offset into array...)
		}
		else appendscriptvalue(0);

		// now at target VAR...
		getlabel();	

		checkforkeyword();
		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		if (tw == concmd_getplayer && aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
		{
			errorcount++;
			ReportError(ERROR_VARREADONLY);
			return 0;

		}
		appendscriptvalue(i);

		break;
	}
	case concmd_setuserdef:
	case concmd_getuserdef:
	{
		int lLabelID;
		// syntax [gs]etuserdef.x <VAR>
		// gets the value of ud.xxx into <VAR>

			// now get name of .xxx
		while (*textptr != '.')
		{
			if (*textptr == 0xa)
				break;
			if (!*textptr)
				break;

			textptr++;
		}
		if (*textptr != '.')
		{
			errorcount++;
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Syntax error.\n", fn, line_number);
			return 0;

		}
		textptr++;
		/// now pointing at 'xxx'
		getlabel();

		lLabelID = getlabeloffset(userdefslabels, parselabel);
		if (lLabelID == -1)
		{
			errorcount++;
			ReportError(ERROR_SYMBOLNOTRECOGNIZED);
			return 0;
		}
		appendscriptvalue(userdefslabels[lLabelID].lId);

		if (userdefslabels[lLabelID].flags & LABEL_HASPARM2)
		{
			// get parm2
		// get the ID of the DEF
			getlabel();	
			checkforkeyword();

			i = GetDefID(parselabel);
			if (i < 0)
			{	// not a defined DEF
				errorcount++;
				ReportError(ERROR_NOTAGAMEDEF);
				return 0;
			}
			appendscriptvalue(i);	// the ID of the DEF (offset into array...)
		}
		else appendscriptvalue(0);


		// now at target VAR...

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		if (tw == concmd_getplayer && aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
		{
			errorcount++;
			ReportError(ERROR_VARREADONLY);
			return 0;

		}
		appendscriptvalue(i);

		break;
	}
	case concmd_setactorvar:
	case concmd_getactorvar:
	{
		// syntax [gs]etactorvar[<var>].<varx> <VAR>
		// gets the value of the per-actor variable varx into VAR

		// now get name of <var>
		while ((*textptr != '['))
		{
			textptr++;
		}
		if (*textptr == '[')
			textptr++;

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		appendscriptvalue(i);

		// now get name of .<varx>
		while (*textptr != '.')
		{
			if (*textptr == 0xa)
				break;
			if (!*textptr)
				break;

			textptr++;
		}
		if (*textptr != '.')
		{
			errorcount++;
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Syntax error.\n", fn, line_number);
			return 0;

		}
		textptr++;
		/// now pointing at 'xxx'
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		if (tw == concmd_setactorvar && aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
		{
			errorcount++;
			ReportError(ERROR_VARREADONLY);
			return 0;

		}
		if (tw == concmd_setactorvar && !(aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR))
		{
			errorcount++;
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Variable '%s' is not per-actor.\n", fn, line_number, parselabel.GetChars());
			return 0;

		}
		appendscriptvalue(i);

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}

		if (tw == concmd_getactorvar && aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
		{
			errorcount++;
			ReportError(ERROR_VARREADONLY);
			return 0;

		}

		appendscriptvalue(i);

		break;
	}
	case concmd_setactor:
	case concmd_getactor:
	{
		int lLabelID;
		// syntax getsector[<var>].x <VAR>
		// gets the value of sector [<var>].xxx into <VAR>

		// now get name of .xxx
		while ((*textptr != '['))
		{
			textptr++;
		}
		if (*textptr == '[')
			textptr++;

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		appendscriptvalue(i);

		// now get name of .xxx
		while (*textptr != '.')
		{
			if (*textptr == 0xa)
				break;
			if (!*textptr)
				break;

			textptr++;
		}
		if (*textptr != '.')
		{
			errorcount++;
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Syntax error.\n", fn, line_number);
			return 0;

		}
		textptr++;
		/// now pointing at 'xxx'
		getlabel();

		lLabelID = getlabeloffset(actorlabels, parselabel);
		if (lLabelID == -1)
		{
			errorcount++;
			ReportError(ERROR_SYMBOLNOTRECOGNIZED);
			return 0;
		}
		appendscriptvalue(actorlabels[lLabelID].lId);

		if (actorlabels[lLabelID].flags & LABEL_HASPARM2)
		{
			// get parm2
		// get the ID of the DEF
			getlabel();	
			checkforkeyword();

			i = GetDefID(parselabel);
			if (i < 0)
			{	// not a defined DEF
				errorcount++;
				ReportError(ERROR_NOTAGAMEDEF);

				Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Symbol '%s' is not a Game Definition.\n", fn, line_number, parselabel.GetChars());
				return 0;
			}
			appendscriptvalue(i);	// the ID of the DEF (offset into array...)
		}
		else appendscriptvalue(0);

		// now at target VAR...

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		if (tw == concmd_getactor && aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
		{
			errorcount++;
			ReportError(ERROR_VARREADONLY);
			return 0;

		}
		appendscriptvalue(i);

		break;
	}
	case concmd_espawnvar:
		// syntax: espawnvar <Var1>
		// spawns the sprite of type ID and sets RETURN to spawned sprite ID
		// FALL THROUGH:
	case concmd_lockplayer:
		// syntax: lockplayer	<var1>
		// sets locks the player controls for <var1> ticks

		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		if (aGameVars[i].dwFlags & GAMEVAR_FLAG_READONLY)
		{
			errorcount++;
			ReportError(ERROR_VARREADONLY);
			return 0;

		}
		appendscriptvalue(i);	// the ID of the DEF (offset into array...)

		return 0;

	case concmd_enhanced:
	{
		popscriptvalue();
		transnum(LABEL_DEFINE);
		int val = popscriptvalue();
		if (val > VERSIONCHECK)
		{
			Printf(TEXTCOLOR_RED "  * ERROR: This CON Code requires at least Build %d, but we are only Build %d\n", val, (int)VERSIONCHECK);
			errorcount++;
		}
		break;
	}

	case concmd_spgetlotag:
	case concmd_spgethitag:
	case concmd_sectgetlotag:
	case concmd_sectgethitag:
	case concmd_gettexturefloor:
	case concmd_gettextureceiling:
		// no paramaters...
		return 0;
	case concmd_starttrack:
		// one parameter (track#)
		transnum(LABEL_DEFINE);
		return 0;
	case concmd_gettexturewall:
		errorcount++;
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Command  'gettexturewall' is not yet implemented.\n", fn, line_number);
		return 0;
	case concmd_displayrand:
		// syntax: displayrand <var>
		// gets rand (not game rand) into <var>

		// Get The ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		appendscriptvalue(i);	// the ID of the DEF (offset into array...)
		break;
	case concmd_switch:
		checking_switch++; // allow nesting (if other things work)
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEVAR);
			return 0;
		}
		appendscriptvalue(i);

		tempscrptr = scriptpos();
		appendscriptvalue(0); // leave spot for end location (for after processing)
		appendscriptvalue(0); // count of case statements
		casescriptptr = scriptpos();	// the first case's pointer.

		appendscriptvalue(0); // leave spot for 'default' location (null if none)

		j = keyword();
		while (j == 20)	// '//'
		{
			while (*textptr != 0x0a)
				textptr++;

			j = keyword();
		}
		// probably does not allow nesting...


		j = CountCaseStatements();
		if (checking_switch > 1)
		{
			//	sprintf(g_szBuf,"ERROR::%s %d: Checking_switch=",__FILE__,__LINE__, checking_switch);
			//	AddLog(g_szBuf);
		}
		if (j < 0)
		{
			return 1;
		}
		if (tempscrptr)
		{
			setscriptvalue(tempscrptr + 1, j);	// save count of cases
		}

		while (j--)
		{
			// leave room for statements
			appendscriptvalue(0);	// value check
			appendscriptvalue(0); // code offset
		}

		casecount = 0;
		while (parsecommand() == 0);

		casecount = 0;
		if (tempscrptr)
		{
			setscriptvalue(tempscrptr, scriptpos());	// save 'end' location
		}

		casescriptptr = 0;
		// decremented in endswitch.  Don't decrement here...
		//			checking_switch--; // allow nesting (maybe if other things work)
		tempscrptr = 0;
		break;


	case concmd_case:
		//AddLog("Found Case");
	repeatcase:
		popscriptvalue(); // don't save in code
		if (checking_switch < 1)
		{
			errorcount++;
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) case statement when not in switch\n", fn, line_number);
			return 1;
		}
		casecount++;
		transnum(LABEL_DEFINE);

		j = popscriptvalue();
		if (casescriptptr)
		{
			setscriptvalue(casescriptptr + (casecount++), j);
			setscriptvalue(casescriptptr + casecount, scriptpos());
		}
		j = keyword();
		while (j == 20)	// '//'
		{
			while (*textptr != 0x0a)
				textptr++;

			j = keyword();
		}
		if (j == concmd_case)
		{
			transword();	// eat 'case'
			goto repeatcase;
		}
		while (parsecommand() == 0);
		return 0;

	case concmd_default:
		popscriptvalue();	// don't save
		if (casescriptptr != 0 && ScriptCode[casescriptptr] != 0)
		{
			// duplicate default statement
			errorcount++;
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) multiple default statements found in switch\n", fn, line_number);
		}
		if (casescriptptr)
		{
			setscriptvalue(casescriptptr, scriptpos());
		}
		while (parsecommand() == 0);
		break;
	case concmd_endswitch:
		checking_switch--;
		if (checking_switch < 0)
		{
			errorcount++;
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) endswitch without matching switch\n", fn, line_number);
		}
		return 1;	// end of block
		break;
	case concmd_startlevel:
		// start at specified level
		// startlevel <episode> <level>
		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		appendscriptvalue(i);

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		appendscriptvalue(i);
		break;
	case concmd_mapvoxel:
		// map a tilenum to a voxel.
		// syntax: mapvoxel <tilenum> <filename (8.3)>
		popscriptvalue(); // don't save in compiled code

		transnum(LABEL_DEFINE);
		j = popscriptvalue();

		while (*textptr == ' ' || *textptr == '\t') textptr++;

		i = 0;
#if 0
		voxel_map[j].name[i] = 0;
		voxel_map[j].voxnum = -2;	// flag to load later
		while ((i < 12) && (isalnum((uint8_t) * textptr) || *textptr == '.'))
		{
			voxel_map[j].name[i++] = *textptr++;
		}
		voxel_map[j].name[i] = 0;
#endif
		break;
	case concmd_myos:
	case concmd_myospal:
	case concmd_myosx:
	case concmd_myospalx:
		// syntax:
		// int x, int y, int tilenum, int shade, int orientation
		// myospal adds char pal

		// Parse: x

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		appendscriptvalue(i);

		// Parse: Y

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEVAR);
			return 0;
		}
		appendscriptvalue(i);

		// Parse: tilenum

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEVAR);
			return 0;
		}
		appendscriptvalue(i);

		// Parse: shade

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEVAR);
			return 0;
		}
		appendscriptvalue(i);

		// Parse: orientation

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEVAR);
			return 0;
		}
		appendscriptvalue(i);

		if (tw == concmd_myospal || tw == concmd_myospalx)
		{
			// Parse: pal

			// get the ID of the DEF
			getlabel();	
			checkforkeyword();

			i = GetDefID(parselabel);
			if (i < 0)
			{	// not a defined DEF
				errorcount++;
				ReportError(ERROR_NOTAGAMEVAR);
				return 0;
			}
			appendscriptvalue(i);
		}

		break;

	case concmd_getangletotarget:
	case concmd_getactorangle:
	case concmd_setactorangle:
	case concmd_getplayerangle:
	case concmd_setplayerangle:
		// Syntax:	 <command> <var>

		// get the ID of the DEF
		getlabel();	
		checkforkeyword();

		i = GetDefID(parselabel);
		//printf("Label '%s' ID is %d\n",label+(labelcnt<<6), i);
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		appendscriptvalue(i);

		return 0;


	case concmd_shadeto:
		popscriptvalue();
		transnum(LABEL_DEFINE);
		popscriptvalue();
		break;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// split in two to allow multiple CON files.
//
//---------------------------------------------------------------------------

void ConCompiler::compilecon(const char *filenam)
{
	currentsourcefile = fileSystem.FindFile(filenam);
	if (currentsourcefile < 0)
	{
		I_FatalError("%s: Missing con file(s).", filenam);
	}
	Printf("Compiling: '%s'.\n", filenam);
	auto data = fileSystem.GetFileData(currentsourcefile, 1);
	textptr = (char*)data.Data();

	line_number = 1;
	errorcount = warningcount = 0;

	while (parsecommand() == 0);

	if ((errorcount) > 64)
		Printf(TEXTCOLOR_RED  "  * ERROR! Too many errors.");
	else if (warningcount || errorcount)
		Printf(TEXTCOLOR_ORANGE "Found %d warning(s), %d error(s).\n", warningcount, errorcount);
	if (errorcount > 0) I_FatalError("Failed to compile %s", filenam);


	// Install the crosshair toggle messages in the CVAR.
	cl_crosshair.SetToggleMessages(quoteMgr.GetRawQuote(QUOTE_CROSSHAIR_OFF), quoteMgr.GetRawQuote(QUOTE_CROSSHAIR_OFF-1));
}

//==========================================================================
//
// Fallback in case nothing got defined.
//
//==========================================================================

static const char* ConFile(void)
{
	if (userConfig.DefaultCon.IsNotEmpty()) return userConfig.DefaultCon.GetChars();

	// WW2GI anf NAM special con names got introduced by EDuke32.
	// Do we really need these?
	if (isWW2GI())
	{
		if (fileSystem.FindFile("ww2gi.con") >= 0) return "ww2gi.con";
	}

	if (g_gameType & GAMEFLAG_NAM)
	{
		if (fileSystem.FindFile("nam.con") >= 0) return "nam.con";
		if (fileSystem.FindFile("napalm.con") >= 0) return "napalm.con";
	}

	if (g_gameType & GAMEFLAG_NAPALM)
	{
		if (fileSystem.FindFile("napalm.con") >= 0) return "napalm.con";
		if (fileSystem.FindFile("nam.con") >= 0) return "nam.con";
	}

	// This got introduced by EDuke 2.0.
	if (g_gameType & GAMEFLAG_DUKE)
	{
		if (fileSystem.FindFile("eduke.con") >= 0) return "eduke.con";	
	}

	// the other games only use game.con.
	return "game.con";
}

//---------------------------------------------------------------------------
//
// process the music definitions after all map records are set up.
//
//---------------------------------------------------------------------------

void ConCompiler::setmusic()
{
	for (auto& tm : tempMusic)
	{
		auto map = FindMapByIndexOnly(tm.volnum, tm.levlnum);
		if (map) map->music = tm.music;
	}
	tempMusic.Clear();
}

//---------------------------------------------------------------------------
//
// why was this called loadefs?
//
//---------------------------------------------------------------------------

void loadcons()
{
	gs = {};
	gs.respawnactortime = 768;
	gs.bouncemineblastradius = 2500;
	gs.respawnitemtime = 768;
	gs.morterblastradius = 2500;
	gs.numfreezebounces = 3;
	gs.pipebombblastradius = 2500;
	gs.playerfriction = 0xCFD0;
	gs.rpgblastradius = 1780;
	gs.seenineblastradius = 2048;
	gs.shrinkerblastradius = 650;
	gs.gravity = 176;
	gs.tripbombblastradius = 3880;
	gs.int_playerheight = PHEIGHT_DUKE << 8;
	gs.playerheight = PHEIGHT_DUKE;
	gs.displayflags = DUKE3D_NO_WIDESCREEN_PINNING;


	ScriptCode.Clear();
	labels.Clear();

	SortCommands();

	ClearGameEvents();
	ClearGameVars();
	AddSystemVars();

	auto before = I_nsTime();

	ScriptCode.Push(0);
	ConCompiler comp;

	if (fileSystem.FileExists("engine/engine.con"))
	{
		comp.compilecon("engine/engine.con");
	}

	comp.compilecon(ConFile()); //Tokenize

	if (userConfig.AddCons) for (FString& m : *userConfig.AddCons.get())
	{
		comp.compilecon(m);
	}
	ScriptCode.ShrinkToFit();
	labels.ShrinkToFit();
	userConfig.AddCons.reset();
	setscriptvalue(0, scriptpos());

	if (comp.getErrorCount())
	{
		I_FatalError("Failed to compile CONs.");
	}
	else
	{
		auto after = I_nsTime();
		Printf("Compilation time:%.2f ms, Code Size:%u bytes. %u labels. %d/%d Variables.\n", (after-before) / 1000000.,
			(ScriptCode.Size() << 2) - 4,
			labels.Size(),
			0,//iGameVarCount,
			MAXGAMEVARS
		);
	}

	// These can only be retrieved AFTER loading the scripts.
	FinalizeGameVars();
	S_WorldTourMappingsForOldSounds(); // create a sound mapping for World Tour.
	S_CacheAllSounds();
	comp.setmusic();

	// RR must link the last map of E1 to the first map of E2.
	if (isRR()) for (auto& map : mapList)
	{
		if (map->cluster == 1) 
		{
			if (!FindMapByLevelNum(map->levelNumber + 1))
			{
				auto nextmap = FindMapByIndexOnly(map->cluster + 1, 1);
				if (nextmap)
				{
					map->NextMap = nextmap->labelName;
					map->flags |= LEVEL_FORCENOEOG;
				}
			}
		}
	}

	if (isWorldTour())
	{
		// fix broken secret exit in WT's super secret map. 
		// This cannot be done from an RMAPINFO definition because the conditions are too specific and must not override custom maps.
		int num = fileSystem.CheckNumForName("e1l7.map");
		int file = fileSystem.GetFileContainer(num);
		if (file <= fileSystem.GetMaxIwadNum())
		{
			auto maprec = FindMapByName("e1l7");
			if (maprec) maprec->NextMap = "e1l5";
		}
	}
}

END_DUKE_NS
