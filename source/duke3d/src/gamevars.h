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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#ifndef gamevars_h_
#define gamevars_h_

#include "fix16.hpp"
#include "gamedef.h"

#include "vfs.h"

#define MAXGAMEVARS 2048 // must be a power of two
#define MAXVARLABEL 26

#define GV_FLAG_CONSTANT (MAXGAMEVARS)
#define GV_FLAG_NEGATIVE (MAXGAMEVARS<<1)
#define GV_FLAG_ARRAY (MAXGAMEVARS<<2)
#define GV_FLAG_STRUCT (MAXGAMEVARS<<3)

// store global game definitions
enum GamevarFlags_t
{
    GAMEVAR_PERPLAYER = 0x00000001,  // per-player variable
    GAMEVAR_PERACTOR  = 0x00000002,  // per-actor variable
    GAMEVAR_USER_MASK = (GAMEVAR_PERPLAYER | GAMEVAR_PERACTOR),
    GAMEVAR_RESET     = 0x00000008,  // INTERNAL, don't use
    GAMEVAR_DEFAULT   = 0x00000100,  // UNUSED, but always cleared for user-defined gamevars
    GAMEVAR_NODEFAULT = 0x00000400,  // don't reset on actor spawn
    GAMEVAR_SYSTEM    = 0x00000800,  // cannot change mode flags...(only default value)
    GAMEVAR_READONLY  = 0x00001000,  // values are read-only (no setvar allowed)
    GAMEVAR_INT32PTR  = 0x00002000,  // plValues is a pointer to an int32_t
    GAMEVAR_INT16PTR  = 0x00008000,  // plValues is a pointer to a short
    GAMEVAR_NORESET   = 0x00020000,  // var values are not reset when restoring map state
    GAMEVAR_SPECIAL   = 0x00040000,  // flag for structure member shortcut vars
    GAMEVAR_NOMULTI   = 0x00080000,  // don't attach to multiplayer packets
    GAMEVAR_Q16PTR    = 0x00100000,  // plValues is a pointer to a q16.16
    GAMEVAR_SERIALIZE = 0x00200000,  // write into permasaves

    GAMEVAR_RAWQ16PTR = GAMEVAR_Q16PTR | GAMEVAR_SPECIAL,  // plValues is a pointer to a q16.16 but we don't want conversion
    GAMEVAR_PTR_MASK  = GAMEVAR_INT32PTR | GAMEVAR_INT16PTR | GAMEVAR_Q16PTR | GAMEVAR_RAWQ16PTR,
};

#if !defined LUNATIC

// Alignments for per-player and per-actor variables.
#define PLAYER_VAR_ALIGNMENT (sizeof(intptr_t))
#define ACTOR_VAR_ALIGNMENT 16

#define ARRAY_ALIGNMENT 16

# define MAXGAMEARRAYS (MAXGAMEVARS>>2) // must be strictly smaller than MAXGAMEVARS
# define MAXARRAYLABEL MAXVARLABEL

enum GamearrayFlags_t
{
    GAMEARRAY_RESET     = 0x00000008,
    GAMEARRAY_RESTORE   = 0x00000010,
    GAMEARRAY_VARSIZE   = 0x00000020,
    GAMEARRAY_STRIDE2   = 0x00000100,
    GAMEARRAY_ALLOCATED = 0x00000200,  // memory allocated for user array
    GAMEARRAY_SYSTEM    = 0x00000800,
    GAMEARRAY_READONLY  = 0x00001000,
    GAMEARRAY_INT16     = 0x00004000,
    GAMEARRAY_INT8      = 0x00008000,
    GAMEARRAY_UNSIGNED  = 0x00010000,
    GAMEARRAY_UINT16    = GAMEARRAY_INT16 | GAMEARRAY_UNSIGNED,
    GAMEARRAY_UINT8     = GAMEARRAY_INT8 | GAMEARRAY_UNSIGNED,
    GAMEARRAY_BITMAP    = 0x00100000,
    GAMEARRAY_WARN      = 0x00200000,

    GAMEARRAY_SIZE_MASK = GAMEARRAY_INT8 | GAMEARRAY_INT16 | GAMEARRAY_BITMAP,
    GAMEARRAY_STORAGE_MASK = GAMEARRAY_INT8 | GAMEARRAY_INT16 | GAMEARRAY_BITMAP | GAMEARRAY_STRIDE2,
    GAMEARRAY_TYPE_MASK = GAMEARRAY_UNSIGNED | GAMEARRAY_INT8 | GAMEARRAY_INT16 | GAMEARRAY_BITMAP,
};

#pragma pack(push,1)
typedef struct
{
    union {
        intptr_t  global;
        intptr_t *pValues;  // array of values when 'per-player', or 'per-actor'
    };
    intptr_t  defaultValue;
    uintptr_t flags;
    char *    szLabel;
} gamevar_t;

typedef struct
{
    char *    szLabel;
    intptr_t *pValues;  // array of values
    intptr_t  size;
    uintptr_t flags;
} gamearray_t;
#pragma pack(pop)

extern gamevar_t   aGameVars[MAXGAMEVARS];
extern gamearray_t aGameArrays[MAXGAMEARRAYS];
extern int32_t     g_gameVarCount;
extern int32_t     g_gameArrayCount;

size_t __fastcall Gv_GetArrayAllocSizeForCount(int const arrayIdx, size_t const count);
size_t __fastcall Gv_GetArrayCountForAllocSize(int const arrayIdx, size_t const filelength);
static FORCE_INLINE size_t Gv_GetArrayAllocSize(int const arrayIdx) { return Gv_GetArrayAllocSizeForCount(arrayIdx, aGameArrays[arrayIdx].size); }
unsigned __fastcall Gv_GetArrayElementSize(int const arrayIdx);
int __fastcall Gv_GetArrayValue(int const id, int index);
int __fastcall Gv_GetVar(int const gameVar, int const spriteNum, int const playerNum);
void __fastcall Gv_SetVar(int const gameVar, int const newValue, int const spriteNum, int const playerNum);
int __fastcall Gv_GetVar(int const gameVar);
void __fastcall Gv_GetManyVars(int const numVars, int32_t * const outBuf);
void __fastcall Gv_SetVar(int const gameVar, int const newValue);

template <typename T>
static FORCE_INLINE void Gv_FillWithVars(T & rv)
{
    EDUKE32_STATIC_ASSERT(sizeof(T) % sizeof(int32_t) == 0);
    EDUKE32_STATIC_ASSERT(sizeof(T) > sizeof(int32_t));
    Gv_GetManyVars(sizeof(T)/sizeof(int32_t), (int32_t *)&rv);
}

int Gv_GetVarByLabel(const char *szGameLabel,int defaultValue,int spriteNum,int playerNum);
void Gv_NewArray(const char *pszLabel,void *arrayptr,intptr_t asize,uint32_t dwFlags);
void Gv_NewVar(const char *pszLabel,intptr_t lValue,uint32_t dwFlags);

static FORCE_INLINE void A_ResetVars(int const spriteNum)
{
    for (auto &gv : aGameVars)
    {
        if ((gv.flags & (GAMEVAR_PERACTOR|GAMEVAR_NODEFAULT)) == GAMEVAR_PERACTOR)
            gv.pValues[spriteNum] = gv.defaultValue;
    }
}
void scriptInitStructTables(void);
void Gv_DumpValues(void);
void Gv_InitWeaponPointers(void);
void Gv_RefreshPointers(void);
void Gv_ResetVars(void);
int Gv_ReadSave(buildvfs_kfd kFile);
void Gv_WriteSave(buildvfs_FILE fil);
void Gv_Clear(void);
#else
extern int32_t g_noResetVars;
extern LUNATIC_CB void (*A_ResetVars)(int32_t spriteNum);
#endif

void Gv_ResetSystemDefaults(void);
void Gv_Init(void);
void Gv_FinalizeWeaponDefaults(void);

#if !defined LUNATIC
static inline int __fastcall VM_GetStruct(uint32_t const flags, intptr_t * const addr)
{
    Bassert(flags & (LABEL_CHAR|LABEL_SHORT|LABEL_INT));

    int returnValue = 0;

    switch (flags & (LABEL_CHAR|LABEL_SHORT|LABEL_INT|LABEL_UNSIGNED))
    {
        case LABEL_CHAR:                 returnValue = *(int8_t *)addr; break;
        case LABEL_CHAR|LABEL_UNSIGNED:  returnValue = *(uint8_t *)addr; break;

        case LABEL_SHORT:                returnValue = *(int16_t *)addr; break;
        case LABEL_SHORT|LABEL_UNSIGNED: returnValue = *(uint16_t *)addr; break;

        case LABEL_INT:                  returnValue = *(int32_t *)addr; break;
        case LABEL_INT|LABEL_UNSIGNED:   returnValue = *(uint32_t *)addr; break;
    }

    return returnValue;
}

static FORCE_INLINE void __fastcall VM_SetStruct(uint32_t const flags, intptr_t * const addr, int32_t newValue)
{
    Bassert(flags & (LABEL_CHAR|LABEL_SHORT|LABEL_INT));

    switch (flags & (LABEL_CHAR|LABEL_SHORT|LABEL_INT|LABEL_UNSIGNED))
    {
        case LABEL_CHAR:                 *(int8_t *)addr = newValue; break;
        case LABEL_CHAR|LABEL_UNSIGNED:  *(uint8_t *)addr = newValue; break;

        case LABEL_SHORT:                *(int16_t *)addr = newValue; break;
        case LABEL_SHORT|LABEL_UNSIGNED: *(uint16_t *)addr = newValue; break;

        case LABEL_INT:                  *(int32_t *)addr = newValue; break;
        case LABEL_INT|LABEL_UNSIGNED:   *(uint32_t *)addr = newValue; break;
    }
}

#define VM_GAMEVAR_OPERATOR(func, operator)                                                  \
    static FORCE_INLINE void __fastcall func(int const id, int32_t const operand)            \
    {                                                                                        \
        auto &var = aGameVars[id];                                                           \
                                                                                             \
        switch (var.flags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK))                          \
        {                                                                                    \
            default: var.global operator operand; break;                                     \
            case GAMEVAR_PERPLAYER:                                                          \
                if (EDUKE32_PREDICT_FALSE((unsigned)vm.playerNum > MAXPLAYERS - 1))          \
                    break;                                                                   \
                var.pValues[vm.playerNum] operator operand;                                  \
                break;                                                                       \
            case GAMEVAR_PERACTOR:                                                           \
                if (EDUKE32_PREDICT_FALSE((unsigned)vm.spriteNum > MAXSPRITES - 1))          \
                    break;                                                                   \
                var.pValues[vm.spriteNum] operator operand;                                  \
                break;                                                                       \
            case GAMEVAR_INT32PTR: *(int32_t *)var.pValues operator(int32_t) operand; break; \
            case GAMEVAR_INT16PTR: *(int16_t *)var.pValues operator(int16_t) operand; break; \
            case GAMEVAR_Q16PTR:                                                             \
            {                                                                                \
                Fix16 *pfix = (Fix16 *)var.global;                                           \
                *pfix operator fix16_from_int(operand);                                      \
                break;                                                                       \
            }                                                                                \
        }                                                                                    \
    }

static FORCE_INLINE void __fastcall Gv_DivVar(int const id, int32_t const operand)
{
    auto &var = aGameVars[id];

    if (EDUKE32_PREDICT_FALSE((var.flags & GAMEVAR_PERPLAYER && (unsigned) vm.playerNum > MAXPLAYERS - 1) ||
        (var.flags & GAMEVAR_PERACTOR && (unsigned) vm.spriteNum > MAXSPRITES - 1)))
        return;

    bool const foundInTable = (unsigned) operand < DIVTABLESIZE;
    static libdivide::libdivide_s32_t sdiv;
    intptr_t *iptr = &var.global;
    static int32_t lastValue;
    auto dptr = foundInTable ? (libdivide::libdivide_s32_t *) &divtable32[operand] : &sdiv;

    if (operand == lastValue || foundInTable)
        goto skip;

    sdiv = libdivide::libdivide_s32_gen((lastValue = operand));

skip:
    switch (var.flags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK))
    {
        case GAMEVAR_PERACTOR: iptr = &var.pValues[vm.spriteNum]; break;
        case GAMEVAR_PERPLAYER: iptr = &var.pValues[vm.playerNum];
        default: break;

        case GAMEVAR_INT32PTR:
        {
            int32_t &value = *(int32_t *)var.pValues;
            value = (int32_t)libdivide::libdivide_s32_do(value, dptr);
            return;
        }
        case GAMEVAR_INT16PTR:
        {
            int16_t &value = *(int16_t *)var.pValues;
            value = (int16_t)libdivide::libdivide_s32_do(value, dptr);
            return;
        }
        case GAMEVAR_Q16PTR:
        {
            fix16_t &value = *(fix16_t *)var.pValues;
            value = fix16_div(value, fix16_from_int(operand));
            return;
        }
    }

    *iptr = libdivide_s32_do(*iptr, dptr);
}

VM_GAMEVAR_OPERATOR(Gv_AddVar, +=)
VM_GAMEVAR_OPERATOR(Gv_SubVar, -=)
VM_GAMEVAR_OPERATOR(Gv_MulVar, *=)
VM_GAMEVAR_OPERATOR(Gv_ModVar, %=)
VM_GAMEVAR_OPERATOR(Gv_AndVar, &=)
VM_GAMEVAR_OPERATOR(Gv_XorVar, ^=)
VM_GAMEVAR_OPERATOR(Gv_OrVar, |=)
VM_GAMEVAR_OPERATOR(Gv_ShiftVarL, <<=)
VM_GAMEVAR_OPERATOR(Gv_ShiftVarR, >>=)

#undef VM_GAMEVAR_OPERATOR

#endif

#endif
