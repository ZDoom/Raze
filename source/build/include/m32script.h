//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2004, 2007 - EDuke32 developers

This file is part of EDuke32

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

#ifndef _m32script_h_
#define _m32script_h_

#include "compat.h"
#include "baselayer.h"
#include "build.h"
#include "editor.h"

#define MAXQUOTES 2048
#define MAXQUOTELEN 128

typedef int32_t instype;
typedef int32_t ofstype;

extern char *apStrings[MAXQUOTES+1], *apXStrings[MAXQUOTES+1];
extern int32_t g_numQuoteRedefinitions;

extern int32_t VM_Execute(int32_t once);
extern void VM_OnEvent(int32_t iEventID, int32_t iActor);

extern void VM_ScriptInfo(void);
extern void VM_Disasm(ofstype beg, int32_t size);

void Gv_NewVar(const char *pszLabel, intptr_t lValue, uint32_t dwFlags);
void Gv_NewArray(const char *pszLabel, void *arrayptr, intptr_t asize, uint32_t dwFlags);
extern void Gv_Init(void);

extern int32_t __fastcall Gv_GetVarX(int32_t id);
extern void __fastcall Gv_SetVarX(int32_t id, int32_t lValue);
extern int32_t __fastcall Gv_GetVarN(int32_t id);  // 'N' for "no side-effects"... vars and locals only!

extern void SetGamePalette(int32_t);

extern int32_t *constants, constants_allocsize;
extern int32_t g_numSavedConstants;

extern instype *apScript ,*insptr;
extern int32_t *labelval;
extern uint8_t *labeltype;
extern int32_t g_numLabels, g_numDefaultLabels;
extern int32_t g_scriptSize;
extern char *label;
//extern int32_t label_allocsize;

extern hashtable_t h_labels;

#define MAXLABELLEN 32

//extern uint8_t waterpal[768],slimepal[768],titlepal[768],drealms[768],endingpal[768],animpal[768];
//extern char currentboardfilename[BMAVM_PATH];


enum GameEvent_t {
    EVENT_ENTER3DMODE,
    EVENT_ANALYZESPRITES,
    EVENT_INSERTSPRITE2D,
    EVENT_INSERTSPRITE3D,
    EVENT_DRAW2DSCREEN,
    EVENT_DRAW3DSCREEN,
    EVENT_KEYS2D,
    EVENT_KEYS3D,
    EVENT_PREKEYS2D,
    EVENT_PREKEYS3D,
    EVENT_LINKTAGS,
    EVENT_KEYPRESS,
    EVENT_PREDRAW3DSCREEN,
    EVENT_LOADMAP,
    EVENT_SAVEMAP,
    EVENT_PRELOADMAP,
    EVENT_PRESAVEMAP,
    EVENT_PREDRAW2DSCREEN,
    MAXEVENTS
};

extern ofstype aEventOffsets[MAXEVENTS];
extern int32_t aEventSizes[MAXEVENTS];
extern uint8_t aEventEnabled[MAXEVENTS];
extern uint16_t aEventNumLocals[MAXEVENTS];


enum GamevarFlags_t {
    MAXGAMEVARS        = 1024,       // must be a power of two between 256 and 4096, inclusive
    LOG2MAXGV          = 10,
    MAXVARLABEL        = MAXLABELLEN, //26,

    GAMEVAR_PERBLOCK   = 0x00000001, // per-block (state, event, or top-level) variable
    GAMEVAR_USER_MASK = GAMEVAR_PERBLOCK,

    GAMEVAR_RESET      = 0x00000008, // marks var for to default

    GAMEVAR_SYSTEM     = 0x00000800, // cannot change mode flags...(only default value)
    GAMEVAR_READONLY   = 0x00001000, // values are read-only (no setvar allowed)

    GAMEVAR_INTPTR     = 0x00002000, // plValues is a pointer to an int32_t
    GAMEVAR_FLOATPTR   = 0x00004000, // plValues is a pointer to a float
    GAMEVAR_SHORTPTR   = 0x00008000, // plValues is a pointer to a short
    GAMEVAR_CHARPTR    = 0x00010000, // plValues is a pointer to a char
    GAMEVAR_PTR_MASK = GAMEVAR_INTPTR|GAMEVAR_FLOATPTR|GAMEVAR_SHORTPTR|GAMEVAR_CHARPTR,

    GAMEVAR_SPECIAL    = 0x00040000, // flag for structure member shortcut vars
};

enum GamearrayFlags_t
{
    MAXGAMEARRAYS = (MAXGAMEVARS >> 2),  // must be strictly smaller than MAXGAMEVARS
    MAXARRAYLABEL = MAXVARLABEL,

    GAMEARRAY_READONLY = 0x00001000,
    GAMEARRAY_WARN     = 0x00002000,

    GAMEARRAY_NORMAL    = 0x00004000,
    GAMEARRAY_UINT8     = 0x00000001,
    GAMEARRAY_INT16     = 0x00000002,
    GAMEARRAY_INT32     = 0x00000004,
    GAMEARRAY_TYPE_MASK = GAMEARRAY_UINT8 | GAMEARRAY_INT16 | GAMEARRAY_INT32,

    GAMEARRAY_RESET = 0x00000008,

    GAMEARRAY_VARSIZE = 0x00000020,
    GAMEARRAY_STRIDE2 = 0x00000040,
};

typedef struct {
    union {
        intptr_t lValue;   // pointer when (dwFlags & GAMEVAR_*PTR)
        int32_t *plValues;     // array of values when (dwFlags & GAMEVAR_PERBLOCK)
    } val;
    intptr_t lDefault;
    char *szLabel;
    uint32_t dwFlags;
} gamevar_t;

typedef struct {
    char *szLabel;
    void *vals;     // array of values, type determined by (dwFlags & GAMEARRAY_TYPEMASK)
    uint32_t dwFlags;
    int32_t size;  // id to size gamevar when (dwFlags & GAMEARRAY_VARSIZE)
} gamearray_t;

extern gamevar_t aGameVars[MAXGAMEVARS];
extern gamearray_t aGameArrays[MAXGAMEARRAYS];
extern int32_t g_gameVarCount, g_systemVarCount;
extern int32_t g_gameArrayCount, g_systemArrayCount;

extern uint32_t m32_drawlinepat;


extern int32_t g_iReturnVar;
extern int32_t g_doScreenShot;
extern int32_t m32_sortvar1, m32_sortvar2;

//extern int32_t g_scriptDebug;

extern int32_t g_numQuoteRedefinitions;

extern hashtable_t h_gamevars;
extern hashtable_t h_arrays;
//extern hashtable_t h_keywords;
extern hashtable_t h_gamefuncs;


extern int32_t mousxplc;
extern int32_t mousyplc;


// gamevar bytecode format:

//  FEDC|BA98|7654|3210|FEDC|BA98|7654|3120
//                     |       .. .... ....  gamevar ID
//                     |      .              constant bit (checked first) / get-payload-var bit for array or struct
//                     |     .               negate bit
//                     |   .                 array bit  \___\  if both set:
//                     |  .                  struct bit /   /  local var
//  .... .... .... ....|                     optional payload



#define M32_FLAG_CONSTANT (MAXGAMEVARS)
#define M32_FLAG_NEGATE (MAXGAMEVARS<<1)

#define M32_FLAG_VAR (0)
#define M32_FLAG_ARRAY (MAXGAMEVARS<<2)
#define M32_FLAG_STRUCT (MAXGAMEVARS<<3)
#define M32_FLAG_LOCAL (M32_FLAG_ARRAY|M32_FLAG_STRUCT)
#define M32_VARTYPE_MASK (M32_FLAG_ARRAY|M32_FLAG_STRUCT)

#define M32_FLAG_CONSTANTINDEX M32_FLAG_CONSTANT
// if set, fetch index for array or struct array from 16 high bits as a constant (otherwise: gamevar)

#define M32_BITS_MASK (0x0000ffff-(MAXGAMEVARS-1))

// IDs of special vars
#define M32_SPRITE_VAR_ID 0
#define M32_SECTOR_VAR_ID 1
#define M32_WALL_VAR_ID 2
#define M32_TSPRITE_VAR_ID 3
#define M32_LIGHT_VAR_ID 4

#define M32_THISACTOR_VAR_ID 5
#define M32_RETURN_VAR_ID 6
#define M32_LOTAG_VAR_ID 7
#define M32_HITAG_VAR_ID 8
#define M32_TEXTURE_VAR_ID 9
#define M32_DOSCRSHOT_VAR_ID 10

#define M32_LOCAL_ARRAY_ID 0

#define M32_PRINTERROR(Text, ...) OSD_Printf("%sLine %d, %s: " Text "\n", osd->draw.errorfmt, g_errorLineNum, keyw[g_tw], ## __VA_ARGS__)
#define M32_ERROR(Text, ...) do { M32_PRINTERROR(Text, ## __VA_ARGS__); vm.flags |= VMFLAG_ERROR; } while (0)


// how local gamevars are allocated:

// uncomment if variable-length arrays are available
//#define M32_LOCALS_VARARRAY

// if neither is there, use a constant number of them
#define M32_LOCALS_FIXEDNUM 64

#if defined M32_LOCALS_VARARRAY
# define M32_MAX_LOCALS MAXGAMEVARS
#else
# define M32_MAX_LOCALS M32_LOCALS_FIXEDNUM
#endif

static inline int32_t Gv_GetArraySize(int32_t id)
{
    if (aGameArrays[id].dwFlags & GAMEARRAY_VARSIZE)
        return Gv_GetVarN(aGameArrays[id].size);

    return aGameArrays[id].size;
}

#endif
