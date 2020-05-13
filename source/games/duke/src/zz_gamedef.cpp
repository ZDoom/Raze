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



char const * VM_GetKeywordForID(int32_t id)
{
    // do not really need this for now...
    return "<invalid keyword>";
}

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

int GetDefID(char const *label) { return hash_find(&h_gamevars, label); }


void loadcons(const char* filenam);

void C_Compile(const char *fileName)
{
    Bmemset(apScriptGameEvent, 0, sizeof(apScriptGameEvent));

    for (int i=0; i<MAXTILES; i++)
    {
        Bmemset(&g_tile[i], 0, sizeof(tiledata_t));
        g_actorMinMs[i] = 1e308;
    }

    C_InitHashes();
    Gv_Init();

    apScript = (intptr_t *)Xcalloc(1, g_scriptSize * sizeof(intptr_t));
    loadcons(fileName);
}


END_DUKE_NS
