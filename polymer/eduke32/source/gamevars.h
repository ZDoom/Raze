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

#include "gamedef.h"

#define MAXGAMEVARS 2048 // must be a power of two
#define MAXVARLABEL 26

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
    GAMEVAR_INTPTR    = 0x00002000,  // plValues is a pointer to an int32_t
    GAMEVAR_SHORTPTR  = 0x00008000,  // plValues is a pointer to a short
    GAMEVAR_CHARPTR   = 0x00010000,  // plValues is a pointer to a char
    GAMEVAR_PTR_MASK  = (GAMEVAR_INTPTR | GAMEVAR_SHORTPTR | GAMEVAR_CHARPTR),
    GAMEVAR_NORESET   = 0x00020000,  // var values are not reset when restoring map state
    GAMEVAR_SPECIAL   = 0x00040000,  // flag for structure member shortcut vars
    GAMEVAR_NOMULTI   = 0x00080000,  // don't attach to multiplayer packets
};

#if !defined LUNATIC

// Alignments for per-player and per-actor variables.
#define PLAYER_VAR_ALIGNMENT (sizeof(intptr_t))
#define ACTOR_VAR_ALIGNMENT 16

# define MAXGAMEARRAYS (MAXGAMEVARS>>2) // must be strictly smaller than MAXGAMEVARS
# define MAXARRAYLABEL MAXVARLABEL

enum GamearrayFlags_t
{
    GAMEARRAY_READONLY  = 0x00001000,
    GAMEARRAY_WARN      = 0x00002000,
    GAMEARRAY_NORMAL    = 0x00004000,
    GAMEARRAY_OFCHAR    = 0x00000001,
    GAMEARRAY_OFSHORT   = 0x00000002,
    GAMEARRAY_OFINT     = 0x00000004,
    GAMEARRAY_RESET     = 0x00000008,
    GAMEARRAY_TYPE_MASK = GAMEARRAY_OFCHAR | GAMEARRAY_OFSHORT | GAMEARRAY_OFINT,
    GAMEARRAY_RESTORE   = 0x00000010,
    GAMEARRAY_VARSIZE   = 0x00000020,
    GAMEARRAY_STRIDE2   = 0x00000100,
};

#pragma pack(push,1)
typedef struct
{
    union {
        intptr_t  nValue;
        intptr_t *pValues;  // array of values when 'per-player', or 'per-actor'
    };
    intptr_t  nDefault;
    uintptr_t nFlags;
    char *    szLabel;
} gamevar_t;

typedef struct
{
    char *    szLabel;
    intptr_t *pValues;  // array of values
    intptr_t  size;
    intptr_t  nFlags;
} gamearray_t;
#pragma pack(pop)

# define GAR_ELTSZ (sizeof(aGameArrays[0].pValues[0]))

extern gamevar_t   aGameVars[MAXGAMEVARS];
extern gamearray_t aGameArrays[MAXGAMEARRAYS];
extern int32_t     g_gameVarCount;
extern int32_t     g_gameArrayCount;

int32_t __fastcall Gv_GetGameArrayValue(int const id, int index);
int32_t __fastcall Gv_GetVar(int id, int nSprite, int nPlayer);
void __fastcall Gv_SetVar(int const id, int32_t const lValue, int const nSprite, int const nPlayer);
int32_t __fastcall Gv_GetVarX(int id);
void __fastcall Gv_GetManyVars(int const count, int32_t * const rv);
void __fastcall Gv_SetVarX(int const id, int32_t const lValue);

int32_t Gv_GetVarByLabel(const char *szGameLabel,int32_t const lDefault,int32_t const iActor,int32_t const iPlayer);
int32_t Gv_NewArray(const char *pszLabel,void *arrayptr,intptr_t asize,uint32_t dwFlags);
int32_t Gv_NewVar(const char *pszLabel,intptr_t lValue,uint32_t dwFlags);

FORCE_INLINE void A_ResetVars(int const nSprite)
{
    for (int i = 0; i < g_gameVarCount; ++i)
    {
        if ((aGameVars[i].nFlags & (GAMEVAR_PERACTOR | GAMEVAR_NODEFAULT)) != GAMEVAR_PERACTOR)
            continue;
        aGameVars[i].pValues[nSprite] = aGameVars[i].nDefault;
    }
}

void Gv_DumpValues(void);
void Gv_InitWeaponPointers(void);
void Gv_RefreshPointers(void);
void Gv_ResetVars(void);
int32_t Gv_ReadSave(int32_t fil,int32_t newbehav);
void Gv_WriteSave(FILE *fil,int32_t newbehav);
#else
extern int32_t g_noResetVars;
extern LUNATIC_CB void (*A_ResetVars)(int32_t iActor);
#endif

void Gv_ResetSystemDefaults(void);
void Gv_Init(void);
void Gv_FinalizeWeaponDefaults(void);

#if !defined LUNATIC
#define VM_GAMEVAR_OPERATOR(func, operator)                                                                                      \
    FORCE_INLINE void __fastcall func(int const id, int32_t const nValue)                                                        \
    {                                                                                                                            \
        switch (aGameVars[id].nFlags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK))                                                   \
        {                                                                                                                        \
            default: aGameVars[id].nValue operator nValue; break;                                                                \
            case GAMEVAR_PERPLAYER:                                                                                              \
                if (EDUKE32_PREDICT_FALSE((unsigned)vm.g_p > MAXPLAYERS - 1))                                                    \
                    break;                                                                                                       \
                aGameVars[id].pValues[vm.g_p] operator nValue;                                                                   \
                break;                                                                                                           \
            case GAMEVAR_PERACTOR:                                                                                               \
                if (EDUKE32_PREDICT_FALSE((unsigned)vm.g_i > MAXSPRITES - 1))                                                    \
                    break;                                                                                                       \
                aGameVars[id].pValues[vm.g_i] operator nValue;                                                                   \
                break;                                                                                                           \
            case GAMEVAR_INTPTR: *((int32_t *)aGameVars[id].nValue) operator(int32_t) nValue; break;                             \
            case GAMEVAR_SHORTPTR: *((int16_t *)aGameVars[id].nValue) operator(int16_t) nValue; break;                           \
            case GAMEVAR_CHARPTR: *((uint8_t *)aGameVars[id].nValue) operator(uint8_t) nValue; break;                            \
        }                                                                                                                        \
    }

#if defined(__arm__) || defined(LIBDIVIDE_ALWAYS)
FORCE_INLINE void __fastcall Gv_DivVar(int const id, int32_t const nValue)
{
    if (EDUKE32_PREDICT_FALSE((aGameVars[id].nFlags & GAMEVAR_PERPLAYER && (unsigned) vm.g_p > MAXPLAYERS - 1) ||
        (aGameVars[id].nFlags & GAMEVAR_PERACTOR && (unsigned) vm.g_i > MAXSPRITES - 1)))
        return;

    static libdivide_s32_t sdiv;
    static int32_t nLastValue;
    libdivide_s32_t *dptr = ((unsigned) nValue < DIVTABLESIZE) ? (libdivide_s32_t *) &divtable32[nValue] : &sdiv;
    intptr_t *iptr = &aGameVars[id].nValue;

    if (nValue == nLastValue || dptr != &sdiv)
        goto skip;

    sdiv = libdivide_s32_gen((nLastValue = nValue));

skip:
    switch (aGameVars[id].nFlags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK))
    {
        case GAMEVAR_PERPLAYER: iptr = &aGameVars[id].pValues[vm.g_p];
        default: break;
        case GAMEVAR_PERACTOR: iptr = &aGameVars[id].pValues[vm.g_i]; break;
        case GAMEVAR_INTPTR:
            *((int32_t *)aGameVars[id].nValue) =
            (int32_t)libdivide_s32_do(*((int32_t *)aGameVars[id].nValue), dptr);
            return;
        case GAMEVAR_SHORTPTR:
            *((int16_t *)aGameVars[id].nValue) =
            (int16_t)libdivide_s32_do(*((int16_t *)aGameVars[id].nValue), dptr);
            return;
        case GAMEVAR_CHARPTR:
            *((uint8_t *)aGameVars[id].nValue) =
            (uint8_t)libdivide_s32_do(*((uint8_t *)aGameVars[id].nValue), dptr);
            return;
    }

    *iptr = libdivide_s32_do(*iptr, dptr);
}
#else
VM_GAMEVAR_OPERATOR(Gv_DivVar, /= )
#endif

VM_GAMEVAR_OPERATOR(Gv_AddVar, +=)
VM_GAMEVAR_OPERATOR(Gv_SubVar, -=)
VM_GAMEVAR_OPERATOR(Gv_MulVar, *=)
VM_GAMEVAR_OPERATOR(Gv_ModVar, %=)
VM_GAMEVAR_OPERATOR(Gv_AndVar, &=)
VM_GAMEVAR_OPERATOR(Gv_XorVar, ^=)
VM_GAMEVAR_OPERATOR(Gv_OrVar, |=)

#undef VM_GAMEVAR_OPERATOR

#endif

#endif
