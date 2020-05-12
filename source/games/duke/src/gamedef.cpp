//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

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

BEGIN_DUKE_NS

// parser state: todo: turn into a class
char* textptr;
int32_t line_count;

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
		if (*textptr == '\n') line_count++;
		textptr++;
	}
}

void skipblockcomment()
{
	while (*textptr != '*' && textptr[1] != '/')
	{
		if (*textptr == '\n') line_count++;
		if (*textptr == 0) return;	// reached the end of the file
		textptr++;
	}
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
		break;
	}
	return *textptr != 0;
}


END_DUKE_NS
