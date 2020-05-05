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

BEGIN_DUKE_NS

#define MAXGAMEVARS 2048 // must be a power of two
#define MAXVARLABEL 26

#define GV_FLAG_CONSTANT (MAXGAMEVARS)
#define GV_FLAG_NEGATIVE (MAXGAMEVARS<<1)

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

// Alignments for per-player and per-actor variables.
#define PLAYER_VAR_ALIGNMENT (sizeof(intptr_t))
#define ACTOR_VAR_ALIGNMENT 16

#define ARRAY_ALIGNMENT 16

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
#pragma pack(pop)

extern gamevar_t   aGameVars[MAXGAMEVARS];
extern int32_t     g_gameVarCount;

int __fastcall Gv_GetVar(int const gameVar, int const spriteNum, int const playerNum);
void __fastcall Gv_SetVar(int const gameVar, int const newValue, int const spriteNum, int const playerNum);
int __fastcall Gv_GetVar(int const gameVar);
void __fastcall Gv_GetManyVars(int const numVars, int32_t * const outBuf);
void __fastcall Gv_SetVar(int const gameVar, int const newValue);

inline void SetGameVarID(int var, int newval, int pnum, int snum)
{
    Gv_SetVar(var, newval, snum, pnum);
}


template <typename T>
static FORCE_INLINE void Gv_FillWithVars(T & rv)
{
    EDUKE32_STATIC_ASSERT(sizeof(T) % sizeof(int32_t) == 0);
    EDUKE32_STATIC_ASSERT(sizeof(T) > sizeof(int32_t));
    Gv_GetManyVars(sizeof(T)/sizeof(int32_t), (int32_t *)&rv);
}

int Gv_GetVarByLabel(const char *szGameLabel,int defaultValue,int spriteNum,int playerNum);
void Gv_NewVar(const char *pszLabel,intptr_t lValue,uint32_t dwFlags);

static FORCE_INLINE void A_ResetVars(int const spriteNum)
{
    for (auto &gv : aGameVars)
    {
        if ((gv.flags & (GAMEVAR_PERACTOR|GAMEVAR_NODEFAULT)) == GAMEVAR_PERACTOR)
            gv.pValues[spriteNum] = gv.defaultValue;
    }
}
void Gv_DumpValues(void);
void Gv_InitWeaponPointers(void);
void Gv_RefreshPointers(void);
void Gv_ResetVars(void);
int Gv_ReadSave(FileReader &kFile);
void Gv_WriteSave(FileWriter &fil);
void Gv_Clear(void);

void Gv_ResetSystemDefaults(void);
void Gv_Init(void);
void Gv_FinalizeWeaponDefaults(void);

#define VM_GAMEVAR_OPERATOR(func, operator)                                                            \
    static FORCE_INLINE ATTRIBUTE((flatten)) void __fastcall func(int const id, int32_t const operand) \
    {                                                                                                  \
        auto &var = aGameVars[id];                                                                     \
                                                                                                       \
        switch (var.flags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK))                                    \
        {                                                                                              \
            default:                                                                                   \
                var.global operator operand;                                                           \
                break;                                                                                 \
            case GAMEVAR_PERPLAYER:                                                                    \
                var.pValues[vm.playerNum & (MAXPLAYERS-1)] operator operand;                           \
                break;                                                                                 \
            case GAMEVAR_PERACTOR:                                                                     \
                var.pValues[vm.spriteNum & (MAXSPRITES-1)] operator operand;                           \
                break;                                                                                 \
            case GAMEVAR_INT32PTR: *(int32_t *)var.pValues operator(int32_t) operand; break;           \
            case GAMEVAR_INT16PTR: *(int16_t *)var.pValues operator(int16_t) operand; break;           \
            case GAMEVAR_Q16PTR:                                                                       \
            {                                                                                          \
                Fix16 *pfix = (Fix16 *)var.global;                                                     \
                *pfix  operator fix16_from_int(operand);                                               \
                break;                                                                                 \
            }                                                                                          \
        }                                                                                              \
    }

VM_GAMEVAR_OPERATOR(Gv_AddVar, +=)
VM_GAMEVAR_OPERATOR(Gv_SubVar, -=)

#undef VM_GAMEVAR_OPERATOR

END_DUKE_NS
#endif
