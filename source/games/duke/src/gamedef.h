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

#ifndef gamedef_h_
#define gamedef_h_

#include "gamevar.h"
#include "actors.h"
#include "build.h"  // hashtable_t
#include "cheats.h"
#include "common.h"  // tokenlist
#include "player.h"  // projectile_t

BEGIN_DUKE_NS

#define LABEL_HASPARM2  1
#define LABEL_ISSTRING  2

// "magic" number for { and }, overrides line number in compiled code for later detection
#define VM_IFELSE_MAGIC 31337
#define VM_INSTMASK 0xfff
#define VM_DECODE_LINE_NUMBER(xxx) ((int)((xxx) >> 12))

extern intptr_t apScriptGameEvent[];

extern char g_scriptFileName[BMAX_PATH];

extern const uint32_t CheatFunctionFlags[];
extern const uint8_t  CheatFunctionIDs[];

extern int errorcount;
extern int32_t line_number;
extern int32_t g_totalLines;
extern int warningcount;
extern int32_t otherp;

int32_t C_AllocQuote(int32_t qnum);
void C_InitQuotes(void);

extern int32_t g_numProjectiles;

void G_DoGameStartup(const int32_t *params);
void C_DefineMusic(int volumeNum, int levelNum, const char *fileName);

void C_DefineVolumeFlags(int32_t vol, int32_t flags);
void ReportError(int32_t iError);
void loadcons(const char *filenam);

extern int32_t g_errorLineNum;
extern int32_t g_tw;

typedef struct {
    const char* token;
    int32_t val;
} tokenmap_t;

extern char const * VM_GetKeywordForID(int32_t id);


#include "concmd.h"


END_DUKE_NS

#endif // gamedef_h_
