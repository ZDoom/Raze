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
#include "menu.h"
#include "global.h"
#include "m_argv.h"

BEGIN_DUKE_NS

// parser state: todo: turn into a class
char* textptr;
char* label;
int line_number;
int labelcnt;
int errorcount, warningcount;	// was named 'error' and 'warning' which is too generic for public variables and may clash with other code.
int g_currentSourceFile;
intptr_t parsing_actor, parsing_event;
int parsing_state;
int num_squigilly_brackets;
int checking_ifelse;

//G_EXTERN char tempbuf[MAXSECTORS << 1], buf[1024]; todo - move to compile state. tempbuf gets used nearly everywhere as scratchpad memory.
extern char tempbuf[];
extern intptr_t* scriptptr;
extern int* labelcode;
extern intptr_t* apScript;

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

#if 0
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
	ERROR_NOENDSWITCH
};
#endif

enum
{
	ERROR_PARMUNDEFINED = 20,
};

void ReportError(int iError)
{
	const char* fn = fileSystem.GetFileFullName(g_currentSourceFile);
	switch (iError)
	{
	case ERROR_ISAKEYWORD:
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Symbol '%s' is a key word.\n",
			fn, line_number, label + (labelcnt << 6));
		break;
	case ERROR_PARMUNDEFINED:
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Parameter '%s' is undefined.\n",
			fn, line_number, tempbuf);
		break;
	case WARNING_DUPLICATEDEFINITION:
		Printf(TEXTCOLOR_YELLOW "  * WARNING.(%s, line %d) Duplicate definition '%s' ignored.\n",
			fn, line_number, label + (labelcnt << 6));
		break;
	case ERROR_COULDNOTFIND:
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Could not find '%s'.\n",
			fn, line_number, label + (labelcnt << 6));
		break;
	case ERROR_VARREADONLY:
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Variable '%s' is read-only.\n",
			fn, line_number, label + (labelcnt << 6));
		break;
	case ERROR_NOTAGAMEDEF:
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Symbol '%s' is not a Game Definition.\n",
			fn, line_number, label + (labelcnt << 6));
		break;
	case ERROR_NOTAGAMEVAR:
		Printf(TEXTCOLOR_RED "  * ERROR! (%s, line %d) Symbol '%s' is not a defined Game Variable.\n",
			fn, line_number, label + (labelcnt << 6));
		break;
	case ERROR_OPENBRACKET:
		Printf(TEXTCOLOR_RED "  * ERROR! (%s, line %d) Found more '{' than '}' before '%s'.\n",
			fn, line_number, tempbuf);
		break;
	case ERROR_CLOSEBRACKET:
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found more '}' than '{' before '%s'.\n",
			fn, line_number, tempbuf);
		break;
	case ERROR_NOENDSWITCH:
		Printf(TEXTCOLOR_RED "  * ERROR!%s(%s, line %d) Did not find endswitch before '%s'.\n",
			fn, line_number, tempbuf);
		break;

	}
}

//---------------------------------------------------------------------------
//
// binary search for keyword
//
//---------------------------------------------------------------------------

int getkeyword(const char* text)
{
	ptrdiff_t min = 0;
	ptrdiff_t max = countof(cmdList) - 1;
	
	while (min <= max)
	{
		int mid = (min + max) >> 1;
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

int findlabel(const char* text)
{
	for (int j = 0; j < labelcnt; j++)
	{
		if (strcmp(label + (j << 6), text) == 0)
		{
			return j;
		}
	}
	return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ispecial(char c)
{
	if (c == 0x0a)
	{
		line_number++;
		return true;
	}

	// oh joy - we need to skip some more characters here to allow running broken scripts.
	if (c == ' ' || c == 0x0d || c == '(' || c == ')' || c == ',' || c == ';')
		return true;

	return false;
}

bool isaltok(char c)
{
	// isalnum pukes on negative input.
	return c > 0 && (isalnum(c) || c == '{' || c == '}' || c == '/' || c == '*' || c == '-' || c == '_' || c == '.');
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void skiptoendofline()
{
	while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)
		textptr++;
}

void skipwhitespace()
{
	while (*textptr == ' ' || *textptr == '\t' || *textptr == '\r' || *textptr == '\n')
	{
		if (*textptr == '\n') line_number++;
		textptr++;
	}
}

void skipblockcomment()
{
	while (*textptr != '*' || textptr[1] != '/')
	{
		if (*textptr == '\n') line_number++;
		if (*textptr == 0) return;	// reached the end of the file
		textptr++;
	}
	textptr += 2;
}

bool skipcomments()
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

int keyword(void)
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
		tempbuf[i] = *(temptextptr++);
		i++;
	}
	tempbuf[i] = 0;

	return getkeyword(tempbuf);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void getlabel(void)
{
	long i;

	skipcomments();

	while (isalnum(*textptr & 0xff) == 0)
	{
		if (*textptr == 0x0a) line_number++;
		textptr++;
		if (*textptr == 0)
			return;
	}

	i = 0;
	while (ispecial(*textptr) == 0)
		label[(labelcnt << 6) + i++] = *(textptr++);

	label[(labelcnt << 6) + i] = 0;
}

//---------------------------------------------------------------------------
//
// script buffer access wrappers. These are here to reduce the affected code
// when the time comes to refactor the buffer into a dynamic array.
//
//---------------------------------------------------------------------------

#if 0
static void setscriptvalue(int offset, int value)
{
	script[offset] = value;
}

int scriptpos()
{
	return script.Size();
}

static void appendscriptvalue(int value)
{
	script.Push(value);
}

static int popscriptvalue()
{
	int p;
	script.Pop(p);
	return p;
}

void pushlabeladdress()
{
	labelcode.Push(script.Size());
}

void reservescriptspace(int space)
{
	script.Reserve(space);
}

#else

// TRANSITIONAL Helpers to write to the old script buffer while using the new interface. Allows to test the parser before implementing the rest.
void scriptWriteValue(int32_t const value);
void scriptWriteAtOffset(int32_t const value, intptr_t addr);
void scriptWritePointer(intptr_t const value, intptr_t addr);
static void setscriptvalue(int offset, int value)
{
	apScript[offset] = value;
}

static void appendscriptvalue(int value)
{
	*scriptptr++ = value;
}

static int popscriptvalue()
{
	return *--scriptptr;
}

int scriptpos()
{
	return int(scriptptr - apScript);
}

void appendlabeladdress(int offset = 0)
{
	labelcode[labelcnt++] = int(scriptptr - apScript) + offset;
	labelcnt++;
}

void appendlabelvalue(int value)
{
	labelcode[labelcnt++] = value;
	labelcnt++;
}

void reservescriptspace(int space)
{
	scriptptr += space;
}


#endif



//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int transword(void) //Returns its code #
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
		if (l < 31)
		{
			tempbuf[l] = textptr[l];
			l++;
		}
	}
	tempbuf[l] = 0;

	i = getkeyword(tempbuf);
	if (i >= 0)
	{
		appendscriptvalue(i);
		textptr += l;
		return i;
	}

	textptr += l;

	const char* fn = fileSystem.GetFileFullName(g_currentSourceFile);
	if (tempbuf[0] == '{' && tempbuf[1] != 0)
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Expecting a SPACE or CR between '{' and '%s'.\n", fn, line_number, tempbuf + 1);
	else if (tempbuf[0] == '}' && tempbuf[1] != 0)
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Expecting a SPACE or CR between '}' and '%s'.\n", fn, line_number, tempbuf + 1);
	else if (tempbuf[0] == '/' && tempbuf[1] == '/' && tempbuf[2] != 0)
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Expecting a SPACE between '//' and '%s'.\n", fn, line_number, tempbuf + 2);
	else if (tempbuf[0] == '/' && tempbuf[1] == '*' && tempbuf[2] != 0)
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Expecting a SPACE between '/*' and '%s'.\n", fn, line_number, tempbuf + 2);
	else if (tempbuf[0] == '*' && tempbuf[1] == '/' && tempbuf[2] != 0)
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Expecting a SPACE between '*/' and '%s'.\n", fn, line_number, tempbuf + 2);
	else
		Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Keyword expected, got '%s'.\n", fn, line_number, tempbuf + 2);

	errorcount++;
	return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int transnum(void)
{
	int i, l;

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
		if (l < 31)
		{
			tempbuf[l] = textptr[l];
			l++;
		}
	}
	tempbuf[l] = 0;

	if (getkeyword(tempbuf) >= 0)
	{
		errorcount++;
		ReportError(ERROR_ISAKEYWORD);
		textptr += l;
	}


	for (i = 0; i < labelcnt; i++)
	{
		if (strcmp(tempbuf, label + (i << 6)) == 0)
		{
			appendscriptvalue(labelcode[i]);
			textptr += l;
			return labelcode[i];
		}
	}

	if (isdigit(*textptr) == 0 && *textptr != '-')
	{
		ReportError(ERROR_PARMUNDEFINED);
		errorcount++;
		textptr += l;
#ifdef FOR_LATER
		if (GetDefID(tempbuf) >= 0)
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
	bool ishex = (textptr[0] == 0 && tolower(textptr[1]) == 'x') || (textptr[0] == '-' && textptr[1] == 0 && tolower(textptr[2]) == 'x');
	if (*textptr == '-') value = strtoll(textptr, &outp, ishex? 16 : 10);
	else value = strtoull(textptr, &outp, ishex ? 16 : 10);
	if (*outp != 0)
	{
		// conversion was not successful.
	}
	appendscriptvalue(int(value));	// truncate the parsed value to 32 bit.
	textptr += l;
	return int(value);
}

//---------------------------------------------------------------------------
//
// just to reduce some excessive boilerplate. This block reappeared
// endlessly in parsecommand
//
//---------------------------------------------------------------------------

void checkforkeyword()
{
	if (getkeyword(label + (labelcnt << 6)) >= 0)
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

static TArray<char> parsebuffer; // global so that the storage is persistent across calls.

int parsecommand()
{
	const char* fn = fileSystem.GetFileFullName(g_currentSourceFile);
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
			appendlabeladdress();

			parsing_state = 1;

			return 0;
		}

		getlabel();
		checkforkeyword();

		lnum = findlabel(label + (labelcnt << 6));

		if (lnum < 0)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) State '%s' not found.\n", fn, line_number, label + (labelcnt << 6));
			errorcount++;
		}
		appendscriptvalue(labelcode[lnum]);
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
		// syntax: gamevar <var1> <initial value> <flags>
		// defines var1 and sets initial value.
		// flags are used to define usage
		// (see top of this files for flags)
		getlabel();	//GetGameVarLabel();
		// Check to see it's already defined
		popscriptvalue();

		if (getkeyword(label + (labelcnt << 6)) >= 0)
		{
			errorcount++;
			ReportError(ERROR_ISAKEYWORD);
			return 0;
		}

		transnum();	// get initial value
		j = popscriptvalue();

		transnum();	// get flags
		lnum = popscriptvalue();
		AddGameVar(label + (labelcnt << 6), j, lnum & (~(GAMEVAR_FLAG_DEFAULT | GAMEVAR_FLAG_SECRET)));
		return 0;

	case concmd_define:
		getlabel();
		checkforkeyword();
		lnum = findlabel(label + (labelcnt << 6));
		if (lnum >= 0)
		{
			warningcount++;
			ReportError(WARNING_DUPLICATEDEFINITION);
			break;
		}

		transnum();
		i = popscriptvalue();
		if (lnum < 0)
		{
			appendlabelvalue(i);
		}
		popscriptvalue();
		return 0;

	case concmd_palfrom:

		for (j = 0; j < 4; j++)
		{
			if (keyword() == -1)
				transnum();
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
			transnum();

			j = 0;
			while (keyword() == -1)
			{
				transnum();
				popscriptvalue();
				j |= *scriptptr;
			}
			appendscriptvalue(j);
		}
		else
		{
			popscriptvalue();
			getlabel();
			// Check to see it's already defined

			checkforkeyword();

			for (i = 0; i < labelcnt; i++)
				if (strcmp(label + (labelcnt << 6), label + (i << 6)) == 0)
				{
					warningcount++;
					Printf(TEXTCOLOR_RED "  * WARNING.(%s, line %d) Duplicate move '%s' ignored.\n", fn, line_number, label + (labelcnt << 6));
					break;
				}
			if (i == labelcnt)
				appendlabeladdress();
			for (j = 0; j < 2; j++)
			{
				if (keyword() >= 0) break;
				transnum();
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
		transnum(); // Volume Number (0/4)
		popscriptvalue();

		k = *scriptptr - 1;
		if (k == -1) k = MAXVOLUMES;

		if (k >= 0) // if it's background music
		{
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
				mapList[(MAXLEVELS * k) + i].music = parsebuffer.Data();
				textptr += j;
				if (i > MAXLEVELS) break;
				i++;
			}
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

		auto fn = fileSystem.FindFile(parsebuffer.Data());
		if (fn < 0)
		{
			errorcount++;
			ReportError(ERROR_COULDNOTFIND);
			return 0;
		}

		auto data = fileSystem.GetFileData(fn, 1);

		temp_current_file = g_currentSourceFile;
		g_currentSourceFile = fn;

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
		g_currentSourceFile = temp_current_file;

		return 0;
	}

	case concmd_ai:
		if (parsing_actor || parsing_state)
			transnum();
		else
		{
			popscriptvalue();
			getlabel();
			checkforkeyword();

			lnum = findlabel(label + (labelcnt << 6));
			if (lnum >= 0)
			{
				warningcount++;
				Printf(TEXTCOLOR_RED "  * WARNING.(%s, line %d) Duplicate ai '%s' ignored.\n", fn, line_number, label + (labelcnt << 6));
			}
			else appendlabeladdress();

			for (j = 0; j < 3; j++)
			{
				if (keyword() >= 0) break;
				if (j == 2)
				{
					k = 0;
					while (keyword() == -1)
					{
						transnum();
						k |= popscriptvalue();
					}
					appendscriptvalue(k);
					return 0;
				}
				else transnum();
			}
			for (k = j; k < 3; k++)
			{
				appendscriptvalue(0);
			}
		}
		return 0;

	case concmd_action:
		if (parsing_actor || parsing_state)
			transnum();
		else
		{
			popscriptvalue();
			getlabel();
			checkforkeyword();

			lnum = findlabel(label + (labelcnt << 6));
			if (lnum >= 0)
			{
				warningcount++;
				Printf(TEXTCOLOR_RED "  * WARNING.(%s, line %d) Duplicate event '%s' ignored.\n", fn, line_number, label + (labelcnt << 6));
			}
			else appendlabeladdress();

			for (j = 0; j < 5; j++)
			{
				if (keyword() >= 0) break;
				transnum();
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

		if (tw == concmd_useractor)
		{ 
			transnum();
			j = popscriptvalue();
		}

		transnum();
		lnum = popscriptvalue();
#if 1
		g_tile[lnum].execPtr = apScript + parsing_actor;	// TRANSITIONAL should only store an index
		if (tw == concmd_useractor)
		{
			if (j & 1)
				g_tile[lnum].flags |= SFLAG_BADGUY;

			if (j & 2)
				g_tile[lnum].flags |= (SFLAG_BADGUY | SFLAG_BADGUYSTAYPUT);
		}
#else
		actorscrptr[lnum] = parsing_actor;
		actortype[lnum] = j;
#endif

		for (j = 0; j < 4; j++)
		{
			setscriptvalue(parsing_actor + j, 0);
			if (j == 3)
			{
				j = 0;
				while (keyword() == -1)
				{
					transnum();
					
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
				transnum();
				// This code was originally here but is a no-op, because both source and destination are the same here.			
				//*(parsing_actor + j) = *(scriptptr - 1);
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

		transnum();
		popscriptvalue();
		j = *scriptptr;	// type of event
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
		transnum();
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
		else if ((i & 48) == 48)
		{
			Printf(TEXTCOLOR_RED "  * WARNING!(%s, line %d) tried to set cstat %d, using %d instead.\n", fn, line_number, i, i ^ 48);
			i ^= 48;
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
		transnum();
		return 0;

	case concmd_addammo:
	case concmd_addweapon:
	case concmd_sizeto:
	case concmd_sizeat:
	case concmd_debris:
	case concmd_addinventory:
	case concmd_guts:
		transnum();
		transnum();
		return 0;

	case concmd_hitradius:
		transnum();
		transnum();
		transnum();
		transnum();
		transnum();
		break;

	case concmd_else:
		if (checking_ifelse)
		{
			checking_ifelse--;
			tempscrptr = scriptpos();
			scriptptr++; //Leave a spot for the fail location
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
		// syntax: [rand|add|set]var	<var1> <const1>
		// sets var1 to const1
		// adds const1 to var1 (const1 can be negative...)

		// get the ID of the DEF
		getlabel();	//GetGameVarLabel();

		// Check to see if it's a keyword
		checkforkeyword();

		i = GetDefID(label + (labelcnt << 6));
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

		transnum();	// the number to check against...
		return 0;

	case concmd_setvarvar:
	case concmd_addvarvar:
		// syntax: [add|set]varvar <var1> <var2>
		// sets var1 = var2
		// adds var1 and var2 with result in var1

		// get the ID of the DEF
		getlabel();	//GetGameVarLabel();

		checkforkeyword();

		i = GetDefID(label + (labelcnt << 6));
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
		getlabel();	//GetGameVarLabel();
		checkforkeyword();

		i = GetDefID(label + (labelcnt << 6));
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

		// get the ID of the DEF
		getlabel();	//GetGameVarLabel();

		checkforkeyword();

		i = GetDefID(label + (labelcnt << 6));
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEDEF);
			return 0;
		}
		appendscriptvalue(i);	// the ID of the DEF (offset into array...)

		// get the ID of the DEF
		getlabel();	//GetGameVarLabel();

		checkforkeyword();

		i = GetDefID(label + (labelcnt << 6));
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

		// get the ID of the DEF
		getlabel();	//GetGameVarLabel();

		checkforkeyword();
		i = GetDefID(label + (labelcnt << 6));
		if (i < 0)
		{	// not a defined DEF
			errorcount++;
			ReportError(ERROR_NOTAGAMEVAR);
			return 0;
		}
		appendscriptvalue(i);	// the ID of the DEF (offset into array...)

		transnum();	// the number to check against...
		goto if_common;

	case concmd_addlogvar:
		// syntax: addlogvar <var>

		appendscriptvalue(g_currentSourceFile);
		appendscriptvalue(line_number);

		// get the ID of the DEF
		getlabel();	//GetGameVarLabel();

		checkforkeyword();

		i = GetDefID(label + (labelcnt << 6));
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
		appendscriptvalue(g_currentSourceFile);

		// prints the line number in the log file.
		appendscriptvalue(line_number);
		return 0;

	case concmd_ifp:
		j = 0;
		do
		{
			transnum();
			popscriptvalue();
			j |= *scriptptr;
		} while (keyword() == -1);
		appendscriptvalue(j);
		goto if_common;

	case concmd_ifpinventory:
		transnum();
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
		transnum();
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
		tempscrptr = scriptpos();
		reservescriptspace(1); //Leave a spot for the fail location

		skipcomments();
		parsecommand();

		setscriptvalue(tempscrptr, scriptpos());
		if (keyword() == concmd_else)	// only increment checking_ifelse if there actually is an else. Otherwise this would break the entire checking logic and render it non-functional
			checking_ifelse++;
		return 0;

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
		popscriptvalue();
		transnum();
		popscriptvalue();
		j = *scriptptr;
		while (*textptr == ' ' || *textptr == '\t') textptr++;

		i = 0;

		parsebuffer.Clear();
		while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)		// JBF 20040127: end of file checked
		{
			parsebuffer.Push(*textptr);
			textptr++, i++;
		}
		parsebuffer.Push(0);
		gVolumeNames[j] = FStringTable::MakeMacro(parsebuffer.Data(), i);
		return 0;
	case concmd_defineskillname:
		popscriptvalue();
		transnum();
		popscriptvalue();
		j = *scriptptr;
		while (*textptr == ' ') textptr++;

		i = 0;

		parsebuffer.Clear();
		while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)		// JBF 20040127: end of file checked
		{
			parsebuffer.Push(*textptr);
			textptr++, i++;
		}
		parsebuffer.Push(0);
		gSkillNames[j] = FStringTable::MakeMacro(parsebuffer.Data(), i);
		return 0;

	case concmd_definelevelname:
		popscriptvalue();
		transnum();
		popscriptvalue();
		j = *scriptptr;
		transnum();
		popscriptvalue();
		k = *scriptptr;
		while (*textptr == ' ') textptr++;

		i = 0;
		parsebuffer.Clear();
		while (*textptr != ' ' && *textptr != 0x0a && *textptr != 0x0d && *textptr != 0)	// JBF 20040127: end of file checked
		{
			parsebuffer.Push(*textptr);
			textptr++, i++;
		}
		parsebuffer.Push(0);
		mapList[j * MAXLEVELS + k].SetFileName(parsebuffer.Data());

		while (*textptr == ' ') textptr++;

		mapList[j * MAXLEVELS + k].parTime =
			(((*(textptr + 0) - '0') * 10 + (*(textptr + 1) - '0')) * 26 * 60) +
			(((*(textptr + 3) - '0') * 10 + (*(textptr + 4) - '0')) * 26);

		textptr += 5;
		while (*textptr == ' ') textptr++;

		mapList[j * MAXLEVELS + k].designerTime =
			(((*(textptr + 0) - '0') * 10 + (*(textptr + 1) - '0')) * 26 * 60) +
			(((*(textptr + 3) - '0') * 10 + (*(textptr + 4) - '0')) * 26);

		textptr += 5;
		while (*textptr == ' ') textptr++;

		i = 0;

		parsebuffer.Clear();
		while (*textptr != 0x0a && *textptr != 0x0d && *textptr != 0)		// JBF 20040127: end of file checked
		{
			parsebuffer.Push(*textptr);
			textptr++, i++;
		}
		parsebuffer.Push(0);
		mapList[j * MAXLEVELS + k].name = parsebuffer.Data();
		return 0;

	case concmd_definequote:
		popscriptvalue();
		transnum();
		k = *(scriptptr - 1);
		if (k >= MAXQUOTES)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Quote number exceeds limit of %d.\n", line_number, MAXQUOTES);
			errorcount++;
		}
		popscriptvalue();
		i = 0;
		while (*textptr == ' ')
			textptr++;

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
		transnum();
		k = *(scriptptr - 1);
		popscriptvalue();
		i = 0;
		while (*textptr == ' ')
			textptr++;

		parsebuffer.Clear();
		while (*textptr != ' ' && *textptr != 0)		// JBF 20040127: end of file checked
		{
			parsebuffer.Push(*textptr);
			textptr++, i++;
		}
		parsebuffer.Push(0);

		transnum();
		int ps = *(scriptptr - 1);
		popscriptvalue();
		transnum();
		int pe = *(scriptptr - 1);
		popscriptvalue();
		transnum();
		int pr = *(scriptptr - 1);
		popscriptvalue();
		transnum();
		int m = *(scriptptr - 1);
		popscriptvalue();
		transnum();
		int vo = *(scriptptr - 1);
		popscriptvalue();
		S_DefineSound(k, parsebuffer.Data(), ps, pe, pr, m, vo, 1.f);
		return 0;
	}

	case concmd_endevent:
		if (parsing_event == 0)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found 'endevent' without defining 'onevent'.\n", line_number);
			errorcount++;
		}
		//			  else
		{
			if (num_squigilly_brackets > 0)
			{
				Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found more '{' than '}' before 'endevent'.\n", line_number);
				errorcount++;
			}
			parsing_event = 0;
			parsing_actor = 0;
		}

		return 0;

	case concmd_enda:
		if (parsing_actor == 0)
		{
			Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found 'enda' without defining 'actor'.\n", line_number);
			errorcount++;
		}
		//			  else
		{
			if (num_squigilly_brackets > 0)
			{
				Printf(TEXTCOLOR_RED "  * ERROR!(%s, line %d) Found more '{' than '}' before 'enda'.\n", line_number);
				errorcount++;
			}
			parsing_actor = 0;
		}

		return 0;
	case concmd_break:
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
		auto parseone = []() { transnum(); return popscriptvalue(); };
		ud.const_visibility = parseone();
		impact_damage = parseone();
		max_player_health = parseone();
		max_armour_amount = parseone();
		respawnactortime = parseone();
		respawnitemtime = parseone();
		dukefriction = parseone();
		gc = parseone();
		rpgblastradius = parseone();
		pipebombblastradius = parseone();
		shrinkerblastradius = parseone();
		tripbombblastradius = parseone();
		morterblastradius = parseone();
		bouncemineblastradius = parseone();
		seenineblastradius = parseone();

		max_ammo_amount[1] = parseone();
		max_ammo_amount[2] = parseone();
		max_ammo_amount[3] = parseone();
		max_ammo_amount[4] = parseone();
		max_ammo_amount[5] = parseone();
		max_ammo_amount[6] = parseone();
		max_ammo_amount[7] = parseone();
		max_ammo_amount[8] = parseone();
		max_ammo_amount[9] = parseone();
		 max_ammo_amount[11] = parseone();
		if (isRR()) max_ammo_amount[12] = parseone();
		camerashitable = parseone();
		numfreezebounces = parseone();
		freezerhurtowner = parseone();
		spriteqamount = clamp(parseone(), 0, 1024);
		lasermode = parseone();
		if (isRRRA())
		{
			max_ammo_amount[13] = parseone();
			max_ammo_amount[14] = parseone();
			max_ammo_amount[16] = parseone();
		}
		scriptptr++;
	}
	return 0;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// split in two to allow multiple CON files.
//
//---------------------------------------------------------------------------

void compilecon(const char *filenam)
{
	g_currentSourceFile = fileSystem.FindFile(filenam);
	if (g_currentSourceFile < 0)
	{
		I_FatalError("%s: Missing con file(s).", filenam);
	}
	Printf("Compiling: '%s'.\n", filenam);
	auto data = fileSystem.GetFileData(g_currentSourceFile, 1);
	textptr = (char*)data.Data();

	line_number = 1;
	errorcount = warningcount = 0;

	while (parsecommand() == 0);

	if ((errorcount) > 64)
		Printf(TEXTCOLOR_RED  "  * ERROR! Too many errors.");
	else if (warningcount || errorcount)
		Printf(TEXTCOLOR_ORANGE "Found %d warning(s), %d error(s).\n", warningcount, errorcount);
	if (errorcount > 0) I_FatalError("Failed to compile %s", filenam);

}

//---------------------------------------------------------------------------
//
// why was this called loadefs?
//
//---------------------------------------------------------------------------

void loadcons(const char* filenam)
{
	labelcnt = 0;

	SortCommands();

#if 0
	ClearGameEvents();
#endif

	ClearGameVars();
	AddSystemVars();


	//memset(actorscrptr, 0, MAXSPRITES);
	//memset(actortype, 0, MAXSPRITES);

	auto before = I_nsTime();

	scriptptr = apScript + 1;
	compilecon(filenam); //Tokenize

	if (userConfig.AddCons) for (FString& m : *userConfig.AddCons.get())
	{
		compilecon(filenam);
	}
	userConfig.AddCons.reset();
	setscriptvalue(0, scriptpos());

	if (errorcount)
	{
		I_FatalError("\nError in %s.", filenam);
	}
	else
	{
		auto after = I_nsTime();
		Printf("Compilation time:%.2f ms, Code Size:%d bytes. %d labels. %d/%d Variables.\n", (after-before) / 1000000.,
			((scriptptr - apScript) << 2) - 4,
			labelcnt,
			0,//iGameVarCount,
			MAXGAMEVARS
		);
	}

	// These can only be retrieved AFTER loading the scripts.
	InitGameVarPointers();
	ResetSystemDefaults();

}

END_DUKE_NS
