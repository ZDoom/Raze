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

// This object is shared by the editors of *all* Build games!

#include "m32script.h"
#include "m32def.h"
#include "osd.h"
#include "keys.h"
#ifdef POLYMER
#include "polymer.h"
#endif

#define _m32vars_c_
#include "m32structures.cpp"

static void Gv_Clear(void)
{
    // only call this function ONCE...
    int32_t i=(MAXGAMEVARS-1);

    //AddLog("Gv_Clear");

    for (; i>=0; i--)
    {
        DO_FREE_AND_NULL(aGameVars[i].szLabel);

        if (aGameVars[i].dwFlags & GAMEVAR_USER_MASK)
            DO_FREE_AND_NULL(aGameVars[i].val.plValues);

        aGameVars[i].val.lValue = 0;
        aGameVars[i].dwFlags |= GAMEVAR_RESET;

        if (i >= MAXGAMEARRAYS)
            continue;

        gamearray_t *const gar = &aGameArrays[i];

        DO_FREE_AND_NULL(gar->szLabel);

        if (gar->dwFlags & GAMEARRAY_NORMAL)
            DO_FREE_AND_NULL(gar->vals);

        gar->dwFlags |= GAMEARRAY_RESET;
    }

    g_gameVarCount = g_gameArrayCount = 0;

    hash_init(&h_gamevars);
    hash_init(&h_arrays);
}

#define ASSERT_IMPLIES(x, y) Bassert(!(x) || (y))

void Gv_NewArray(const char *pszLabel, void *arrayptr, intptr_t asize, uint32_t dwFlags)
{
    ASSERT_IMPLIES(dwFlags&GAMEARRAY_VARSIZE, dwFlags&GAMEARRAY_READONLY);
    ASSERT_IMPLIES(dwFlags&GAMEARRAY_STRIDE2, dwFlags&GAMEARRAY_READONLY);
    ASSERT_IMPLIES(dwFlags&GAMEARRAY_TYPE_MASK,
                   g_gameArrayCount==0 || (dwFlags&(GAMEARRAY_READONLY|GAMEARRAY_WARN)));

    if (g_gameArrayCount >= MAXGAMEARRAYS)
    {
        C_CUSTOMERROR("too many arrays! (max: %d)", MAXGAMEARRAYS);
        return;
    }

    if (Bstrlen(pszLabel) > (MAXARRAYLABEL-1))
    {
        C_CUSTOMERROR("array name `%s' exceeds limit of %d characters.", pszLabel, MAXARRAYLABEL);
        return;
    }

    const int32_t i = hash_find(&h_arrays, pszLabel);

    if (i>=0 && !(aGameArrays[i].dwFlags & GAMEARRAY_RESET))
    {
        // found it it's a duplicate in error

        if (aGameArrays[i].dwFlags&GAMEARRAY_TYPE_MASK)
            C_CUSTOMWARNING("ignored redefining system array `%s'.", pszLabel);

//        C_ReportError(WARNING_DUPLICATEDEFINITION);
        return;
    }

    if (!(dwFlags&GAMEARRAY_VARSIZE) && !(dwFlags&GAMEARRAY_TYPE_MASK) && (asize<=0 || asize>65536))
    {
        // the dummy array with index 0 sets the size to 0 so that accidental accesses as array
        // will complain.
        C_CUSTOMERROR("invalid array size %d. Must be between 1 and 65536", (int)asize);
        return;
    }

    gamearray_t *const gar = &aGameArrays[g_gameArrayCount];

    if (gar->szLabel == NULL)
        gar->szLabel = (char *)Xcalloc(MAXARRAYLABEL, sizeof(char));
    if (gar->szLabel != pszLabel)
        Bstrcpy(gar->szLabel, pszLabel);

    if (!(dwFlags & GAMEARRAY_TYPE_MASK))
        gar->vals = (int32_t *)Xcalloc(asize, sizeof(int32_t));
    else
        gar->vals = arrayptr;

    gar->size = asize;
    gar->dwFlags = dwFlags & ~GAMEARRAY_RESET;

    hash_add(&h_arrays, gar->szLabel, g_gameArrayCount, 1);
    g_gameArrayCount++;
}

void Gv_NewVar(const char *pszLabel, intptr_t lValue, uint32_t dwFlags)
{
    int32_t i, j;

    //Bsprintf(g_szBuf,"Gv_NewVar(%s, %d, %X)",pszLabel, lValue, dwFlags);
    //AddLog(g_szBuf);

    if (g_gameVarCount >= MAXGAMEVARS)
    {
        C_CUSTOMERROR("too many gamevars! (max: %d)", MAXGAMEVARS);
        return;
    }

    if (Bstrlen(pszLabel) > (MAXVARLABEL-1))
    {
        C_CUSTOMERROR("variable name `%s' exceeds limit of %d characters.", pszLabel, MAXVARLABEL);
        return;
    }

    i = hash_find(&h_gamevars,pszLabel);

    if (i >= 0 && !(aGameVars[i].dwFlags & GAMEVAR_RESET))
    {
        // found it...
        if (aGameVars[i].dwFlags & GAMEVAR_PTR_MASK)
        {
            C_ReportError(-1);
            initprintf("%s:%d: warning: cannot redefine internal gamevar `%s'.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
            return;
        }
        else if (!(aGameVars[i].dwFlags & GAMEVAR_SYSTEM))
        {
            // it's a duplicate in error
//            g_numCompilerWarnings++;
//            C_ReportError(WARNING_DUPLICATEDEFINITION);
            return;
        }
    }

    if (i == -1)
        i = g_gameVarCount;

    // Set values
    if ((aGameVars[i].dwFlags & GAMEVAR_SYSTEM) == 0)
    {
        if (aGameVars[i].szLabel == NULL)
            aGameVars[i].szLabel = (char *)Xcalloc(MAXVARLABEL, sizeof(uint8_t));
        if (aGameVars[i].szLabel != pszLabel)
            Bstrcpy(aGameVars[i].szLabel,pszLabel);
        aGameVars[i].dwFlags = dwFlags;

        if (aGameVars[i].dwFlags & GAMEVAR_USER_MASK)
        {
            // only free if not system
            DO_FREE_AND_NULL(aGameVars[i].val.plValues);
        }
    }

    // if existing is system, they only get to change default value....
    aGameVars[i].lDefault = lValue;
    aGameVars[i].dwFlags &= ~GAMEVAR_RESET;

    if (i == g_gameVarCount)
    {
        // we're adding a new one.
        hash_add(&h_gamevars, aGameVars[i].szLabel, g_gameVarCount++, 0);
    }

    if (aGameVars[i].dwFlags & GAMEVAR_PERBLOCK)
    {
        if (!aGameVars[i].val.plValues)
            aGameVars[i].val.plValues = (int32_t *)Xcalloc(1+MAXEVENTS+g_stateCount, sizeof(int32_t));
        for (j=0; j<1+MAXEVENTS+g_stateCount; j++)
            aGameVars[i].val.plValues[j] = lValue;
    }
    else aGameVars[i].val.lValue = lValue;
}

int32_t __fastcall Gv_GetVarN(int32_t id)  // 'N' for "no side-effects"... vars and locals only!
{
    if (id == M32_THISACTOR_VAR_ID)
        return vm.spriteNum;

    switch (id&M32_VARTYPE_MASK)
    {
    case M32_FLAG_VAR:
        id &= (MAXGAMEVARS-1);

        switch (aGameVars[id].dwFlags & (GAMEVAR_USER_MASK|GAMEVAR_PTR_MASK))
        {
        case 0:
            return aGameVars[id].val.lValue;
        case GAMEVAR_PERBLOCK:
            return aGameVars[id].val.plValues[vm.g_st];
        case GAMEVAR_FLOATPTR:
        case GAMEVAR_INTPTR:
            return *((int32_t *)aGameVars[id].val.lValue);
        case GAMEVAR_SHORTPTR:
            return *((int16_t *)aGameVars[id].val.lValue);
        case GAMEVAR_CHARPTR:
            return *((uint8_t *)aGameVars[id].val.lValue);
        default:
            M32_ERROR("Gv_GetVarN(): WTF??");
            return -1;
        }

    case M32_FLAG_LOCAL:
    {
        int32_t index = id&(MAXGAMEVARS-1);
        // no bounds checking since it's done at script compilation time
        return ((int32_t *)aGameArrays[M32_LOCAL_ARRAY_ID].vals)[index];
    }

    default:
        M32_ERROR("Gv_GetVarN(): invalid var code %0x08x", id);
        return -1;
    }
}

int32_t __fastcall Gv_GetVar(int32_t id)
{
    int32_t negateResult = !!(id&M32_FLAG_NEGATE);

    if (id == M32_THISACTOR_VAR_ID)
        return vm.spriteNum;

    id &= ~M32_FLAG_NEGATE;

    if ((id & M32_BITS_MASK) == M32_FLAG_CONSTANT)
    {
        switch (id&3)
        {
        case 0:
            return ((int16_t)(id>>16));
        case 1:
            return constants[(id>>16)&0xffff];
        case 2:
            return (labelval[(id>>16)&0xffff] ^ -negateResult) + negateResult;
        default:
            M32_ERROR("Gv_GetVarX() (constant): WTF??");
            return -1;
        }
    }


    switch (id&M32_VARTYPE_MASK)
    {
    case M32_FLAG_ARRAY:
    {
        int32_t index;

        index = (int32_t)((id>>16)&0xffff);
        if (!(id&M32_FLAG_CONSTANTINDEX))
            index = Gv_GetVarN(index);

        id &= (MAXGAMEARRAYS-1);

        const int32_t siz = Gv_GetArraySize(id);
        const gamearray_t *const gar = &aGameArrays[id];

        if (index < 0 || index >= siz)
        {
            M32_ERROR("Gv_GetVarX(): invalid array index (%s[%d])", gar->szLabel, index);
            return -1;
        }

        if (gar->dwFlags & GAMEARRAY_STRIDE2)
            index <<= 1;

        switch (gar->dwFlags & GAMEARRAY_TYPE_MASK)
        {
        case 0:
        case GAMEARRAY_INT32:
            return (((int32_t *)gar->vals)[index] ^ -negateResult) + negateResult;
        case GAMEARRAY_INT16:
            return (((int16_t *)gar->vals)[index] ^ -negateResult) + negateResult;
        case GAMEARRAY_UINT8:
            return (((uint8_t *)gar->vals)[index] ^ -negateResult) + negateResult;
        default:
            M32_ERROR("Gv_GetVarX() (array): WTF??");
            return -1;
        }
    }
    case M32_FLAG_STRUCT:
    {
        int32_t index, memberid;

        index = (id>>16)&0x7fff;
        if (!(id&M32_FLAG_CONSTANTINDEX))
            index = Gv_GetVarN(index);

        memberid = (id>>2)&63;

        switch (id&3)
        {
        case M32_SPRITE_VAR_ID:
            return (VM_AccessSprite(0, index, memberid, 0) ^ -negateResult) + negateResult;
        case M32_SECTOR_VAR_ID:
            return (VM_AccessSector(0, index, memberid, 0) ^ -negateResult) + negateResult;
        case M32_WALL_VAR_ID:
            return (VM_AccessWall(0, index, memberid, 0) ^ -negateResult) + negateResult;
        case M32_TSPRITE_VAR_ID:
            return (VM_AccessTsprite(0, index, memberid, 0) ^ -negateResult) + negateResult;
        default:
            M32_ERROR("Gv_GetVarX(): WTF??");
            return -1;
        }
    }
    case M32_FLAG_VAR:
    {
        id &= (MAXGAMEVARS-1);

        switch (aGameVars[id].dwFlags & (GAMEVAR_USER_MASK|GAMEVAR_PTR_MASK))
        {
        case 0:
            return (aGameVars[id].val.lValue ^ -negateResult) + negateResult;
        case GAMEVAR_PERBLOCK:
            return (aGameVars[id].val.plValues[vm.g_st] ^ -negateResult) + negateResult;
        case GAMEVAR_FLOATPTR:
        {
            union { int32_t ival; float fval; };

            fval = *(float *)aGameVars[id].val.plValues;
            if (negateResult)
                fval *= -1;
            return ival;
        }
        case GAMEVAR_INTPTR:
            return (*((int32_t *)aGameVars[id].val.lValue) ^ -negateResult) + negateResult;
        case GAMEVAR_SHORTPTR:
            return (*((int16_t *)aGameVars[id].val.lValue) ^ -negateResult) + negateResult;
        case GAMEVAR_CHARPTR:
            return (*((uint8_t *)aGameVars[id].val.lValue) ^ -negateResult) + negateResult;
        default:
            M32_ERROR("Gv_GetVarX(): WTF??");
            return -1;
        }
    }
    case M32_FLAG_LOCAL:
    {
        int32_t index = id&(MAXGAMEVARS-1);
        // no bounds checking since it's done at script compilation time
        return (((int32_t *)aGameArrays[M32_LOCAL_ARRAY_ID].vals)[index] ^ -negateResult) + negateResult;
    }
    }  // switch (id&M32_VARTYPE_MASK)

    return 0;  // never reached
}


void __fastcall Gv_SetVar(int32_t id, int32_t lValue)
{
    switch (id&M32_VARTYPE_MASK)
    {
    case M32_FLAG_ARRAY:
    {
        int32_t index;

        index = (id>>16)&0xffff;
        if (!(id&M32_FLAG_CONSTANTINDEX))
            index = Gv_GetVarN(index);

        id &= (MAXGAMEARRAYS-1);

        const int32_t siz = Gv_GetArraySize(id);
        gamearray_t *const gar = &aGameArrays[id];

        if (index < 0 || index >= siz)
        {
            M32_ERROR("Gv_SetVarX(): invalid array index %s[%d], size=%d", gar->szLabel, index, siz);
            return;
        }

        // NOTE: GAMEARRAY_READONLY arrays can be modified in expert mode.
        Bassert((gar->dwFlags & GAMEARRAY_STRIDE2) == 0);

        switch (gar->dwFlags & GAMEARRAY_TYPE_MASK)
        {
        case 0:
        case GAMEARRAY_INT32:
            ((int32_t *)gar->vals)[index] = lValue;
            return;
        case GAMEARRAY_INT16:
            ((int16_t *)gar->vals)[index] = (int16_t)lValue;
            return;
        case GAMEARRAY_UINT8:
            ((uint8_t *)gar->vals)[index] = (uint8_t)lValue;
            return;
        default:
            M32_ERROR("Gv_SetVarX() (array): WTF??");
            return;
        }
        return;
    }
    case M32_FLAG_STRUCT:
    {
        int32_t index, memberid;

        index = (id>>16)&0x7fff;
        if (!(id&M32_FLAG_CONSTANTINDEX))
            index = Gv_GetVarN(index);

        memberid = (id>>2)&63;

        switch (id&3)
        {
        case M32_SPRITE_VAR_ID:
            VM_AccessSprite(1, index, memberid, lValue);
            return;
        case M32_SECTOR_VAR_ID:
            VM_AccessSector(1, index, memberid, lValue);
            return;
        case M32_WALL_VAR_ID:
            VM_AccessWall(1, index, memberid, lValue);
            return;
        case M32_TSPRITE_VAR_ID:
            VM_AccessTsprite(1, index, memberid, lValue);
            return;
        default:
            M32_ERROR("Gv_SetVarX(): WTF??");
            return;
        }
    }
    case M32_FLAG_VAR:
    {
        id &= (MAXGAMEVARS-1);

        switch (aGameVars[id].dwFlags & (GAMEVAR_USER_MASK|GAMEVAR_PTR_MASK))
        {
        case 0:
            aGameVars[id].val.lValue=lValue;
            return;
        case GAMEVAR_PERBLOCK:
            aGameVars[id].val.plValues[vm.g_st] = lValue;
            return;
        case GAMEVAR_FLOATPTR:
        {
            union { int32_t ival; float fval; };
            ival = lValue;

            if (fval!=fval || fval<-3.4e38 || fval > 3.4e38)
            {
                M32_ERROR("Gv_SetVarX(): tried to set float var to NaN or infinity");
                return;
            }
        }
        fallthrough__;
        case GAMEVAR_INTPTR:
            *((int32_t *)aGameVars[id].val.lValue)=(int32_t)lValue;
            return;
        case GAMEVAR_SHORTPTR:
            *((int16_t *)aGameVars[id].val.lValue)=(int16_t)lValue;
            return;
        case GAMEVAR_CHARPTR:
            *((uint8_t *)aGameVars[id].val.lValue)=(uint8_t)lValue;
            return;
        default:
            M32_ERROR("Gv_SetVarX(): WTF??");
            return;
        }
    }
    case M32_FLAG_LOCAL:
    {
        int32_t index = id&(MAXGAMEVARS-1);
        ((int32_t *)aGameArrays[M32_LOCAL_ARRAY_ID].vals)[index] = lValue;
        return;
    }
    }
}

static uint8_t alphakeys[] =
{
    KEYSC_SPACE,

    KEYSC_A, KEYSC_B, KEYSC_C, KEYSC_D, KEYSC_E, KEYSC_F, KEYSC_G, KEYSC_H,
    KEYSC_I, KEYSC_J, KEYSC_K, KEYSC_L, KEYSC_M, KEYSC_N, KEYSC_O, KEYSC_P,
    KEYSC_Q, KEYSC_R, KEYSC_S, KEYSC_T, KEYSC_U, KEYSC_V, KEYSC_W, KEYSC_X,
    KEYSC_Y, KEYSC_Z,
};

static uint8_t numberkeys[] =
{
    KEYSC_0, KEYSC_1, KEYSC_2, KEYSC_3, KEYSC_4, KEYSC_5, KEYSC_6, KEYSC_7,
    KEYSC_8, KEYSC_9,
};

static void Gv_AddSystemVars(void)
{
    // only call ONCE
    int32_t hlcnt_id, hlscnt_id;

    // special vars for struct access
    // MUST be at top and in this order!!!
    Gv_NewVar("sprite", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("sector", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("wall", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("tsprite", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("light", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);

    // these too have to be in here and in order!
    // keep in sync with m32script.h: IDs of special vars

    Gv_NewVar("I", 0, GAMEVAR_READONLY | GAMEVAR_SYSTEM);  // THISACTOR
    Gv_NewVar("RETURN", (intptr_t)&g_iReturnVar, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("LOTAG", 0, GAMEVAR_SYSTEM);
    Gv_NewVar("HITAG", 0, GAMEVAR_SYSTEM);
    Gv_NewVar("TEXTURE", 0, GAMEVAR_SYSTEM);
    Gv_NewVar("DOSCRSHOT", (intptr_t)&g_doScreenShot, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);

    Gv_NewVar("xdim",(intptr_t)&xdim, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("ydim",(intptr_t)&ydim, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("windowx1",(intptr_t)&windowxy1.x, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("windowx2",(intptr_t)&windowxy2.x, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("windowy1",(intptr_t)&windowxy1.y, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("windowy2",(intptr_t)&windowxy2.y, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("totalclock",(intptr_t)&totalclock, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);

    Gv_NewVar("viewingrange",(intptr_t)&viewingrange, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("yxaspect",(intptr_t)&yxaspect, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);

///    Gv_NewVar("framerate",(intptr_t)&g_frameRate, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
///    Gv_NewVar("display_mirror",(intptr_t)&display_mirror, GAMEVAR_SYSTEM | GAMEVAR_CHARPTR);

    Gv_NewVar("randomseed",(intptr_t)&randomseed, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);

    Gv_NewVar("numwalls",(intptr_t)&numwalls, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_READONLY);
    Gv_NewVar("numsectors",(intptr_t)&numsectors, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_READONLY);
    Gv_NewVar("numsprites",(intptr_t)&Numsprites, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    {
        static int32_t numtiles;
        Gv_NewVar("numtiles",(intptr_t)&numtiles, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    }
#ifdef YAX_ENABLE
    Gv_NewVar("numbunches",(intptr_t)&numyaxbunches, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
#endif

#ifdef USE_OPENGL
    Gv_NewVar("rendmode",(intptr_t)&rendmode, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
#endif

    // current position
    Gv_NewVar("posx",(intptr_t)&pos.x, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("posy",(intptr_t)&pos.y, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("posz",(intptr_t)&pos.z, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("ang",(intptr_t)&ang, GAMEVAR_SHORTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("horiz",(intptr_t)&horiz, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("cursectnum",(intptr_t)&cursectnum, GAMEVAR_READONLY | GAMEVAR_SHORTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("hardcoded_movement",(intptr_t)&g_doHardcodedMovement, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);

    Gv_NewVar("searchx",(intptr_t)&searchx, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("searchy",(intptr_t)&searchy, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("searchstat",(intptr_t)&searchstat, GAMEVAR_READONLY | GAMEVAR_SHORTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("searchwall",(intptr_t)&searchwall, GAMEVAR_READONLY | GAMEVAR_SHORTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("searchsector",(intptr_t)&searchsector, GAMEVAR_READONLY | GAMEVAR_SHORTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("searchbottomwall",(intptr_t)&searchbottomwall, GAMEVAR_READONLY | GAMEVAR_SHORTPTR | GAMEVAR_SYSTEM);

    Gv_NewVar("pointhighlight",(intptr_t)&pointhighlight, GAMEVAR_SHORTPTR|GAMEVAR_SYSTEM|GAMEVAR_READONLY);
    Gv_NewVar("linehighlight",(intptr_t)&linehighlight, GAMEVAR_SHORTPTR|GAMEVAR_SYSTEM|GAMEVAR_READONLY);

    hlcnt_id = g_gameVarCount;
    Gv_NewVar("highlightcnt",(intptr_t)&highlightcnt, GAMEVAR_SHORTPTR|GAMEVAR_SYSTEM|GAMEVAR_READONLY);
    hlscnt_id = g_gameVarCount;
    Gv_NewVar("highlightsectorcnt",(intptr_t)&highlightsectorcnt, GAMEVAR_SHORTPTR|GAMEVAR_SYSTEM|GAMEVAR_READONLY);

    // clipboard contents
    Gv_NewVar("temppicnum",(intptr_t)&temppicnum, GAMEVAR_INTPTR|GAMEVAR_SYSTEM|GAMEVAR_READONLY);
    Gv_NewVar("tempcstat",(intptr_t)&tempcstat, GAMEVAR_INTPTR|GAMEVAR_SYSTEM|GAMEVAR_READONLY);
    Gv_NewVar("templotag",(intptr_t)&templotag, GAMEVAR_INTPTR|GAMEVAR_SYSTEM|GAMEVAR_READONLY);
    Gv_NewVar("temphitag",(intptr_t)&temphitag, GAMEVAR_INTPTR|GAMEVAR_SYSTEM|GAMEVAR_READONLY);
    Gv_NewVar("tempextra",(intptr_t)&tempextra, GAMEVAR_INTPTR|GAMEVAR_SYSTEM|GAMEVAR_READONLY);
    Gv_NewVar("tempshade",(intptr_t)&tempshade, GAMEVAR_INTPTR|GAMEVAR_SYSTEM|GAMEVAR_READONLY);
    Gv_NewVar("temppal",(intptr_t)&temppal, GAMEVAR_INTPTR|GAMEVAR_SYSTEM|GAMEVAR_READONLY);
    Gv_NewVar("tempvis",(intptr_t)&tempvis, GAMEVAR_INTPTR|GAMEVAR_SYSTEM|GAMEVAR_READONLY);
    Gv_NewVar("tempxrepeat",(intptr_t)&tempxrepeat, GAMEVAR_INTPTR|GAMEVAR_SYSTEM|GAMEVAR_READONLY);
    Gv_NewVar("tempyrepeat",(intptr_t)&tempyrepeat, GAMEVAR_INTPTR|GAMEVAR_SYSTEM|GAMEVAR_READONLY);

    // starting position
    Gv_NewVar("startposx",(intptr_t)&startpos.x, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("startposy",(intptr_t)&startpos.y, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("startposz",(intptr_t)&startpos.z, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("startang",(intptr_t)&startang, GAMEVAR_READONLY | GAMEVAR_SHORTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("startsectnum",(intptr_t)&startsectnum, GAMEVAR_READONLY | GAMEVAR_SHORTPTR | GAMEVAR_SYSTEM);

    Gv_NewVar("mousxplc",(intptr_t)&mousxplc, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("mousyplc",(intptr_t)&mousyplc, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("mousebits",(intptr_t)&g_mouseBits, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);

    Gv_NewVar("zoom",(intptr_t)&zoom, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("drawlinepat",(intptr_t)&m32_drawlinepat, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("halfxdim16", (intptr_t)&halfxdim16, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("midydim16", (intptr_t)&midydim16, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("ydim16",(intptr_t)&ydim16, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("m32_sideview",(intptr_t)&m32_sideview, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);

    Gv_NewVar("SV1",(intptr_t)&m32_sortvar1, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("SV2",(intptr_t)&m32_sortvar2, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("spritesortcnt",(intptr_t)&spritesortcnt, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);

#ifdef POLYMER
    Gv_NewVar("pr_overrideparallax",(intptr_t)&pr_overrideparallax, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("pr_parallaxscale",(intptr_t)&pr_parallaxscale, GAMEVAR_FLOATPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("pr_parallaxbias",(intptr_t)&pr_parallaxbias, GAMEVAR_FLOATPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("pr_overridespecular",(intptr_t)&pr_overridespecular, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("pr_specularpower",(intptr_t)&pr_specularpower, GAMEVAR_FLOATPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("pr_specularfactor",(intptr_t)&pr_specularfactor, GAMEVAR_FLOATPTR | GAMEVAR_SYSTEM);
#else
    {
        // dummy Polymer variables for non-Polymer builds
        static int32_t pr_overrideparallax = 0;
        static float pr_parallaxscale = 0.1f;
        static float pr_parallaxbias = 0.0f;
        static int32_t pr_overridespecular = 0;
        static float pr_specularpower = 15.0f;
        static float pr_specularfactor = 1.0f;

        Gv_NewVar("pr_overrideparallax",(intptr_t)&pr_overrideparallax, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
        Gv_NewVar("pr_parallaxscale",(intptr_t)&pr_parallaxscale, GAMEVAR_FLOATPTR | GAMEVAR_SYSTEM);
        Gv_NewVar("pr_parallaxbias",(intptr_t)&pr_parallaxbias, GAMEVAR_FLOATPTR | GAMEVAR_SYSTEM);
        Gv_NewVar("pr_overridespecular",(intptr_t)&pr_overridespecular, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
        Gv_NewVar("pr_specularpower",(intptr_t)&pr_specularpower, GAMEVAR_FLOATPTR | GAMEVAR_SYSTEM);
        Gv_NewVar("pr_specularfactor",(intptr_t)&pr_specularfactor, GAMEVAR_FLOATPTR | GAMEVAR_SYSTEM);
    }
#endif

    g_systemVarCount = g_gameVarCount;

    // must be first!
    Gv_NewArray(".LOCALS_BASE", NULL, 0, GAMEARRAY_INT32);

    Gv_NewArray("highlight", (void *)highlight, hlcnt_id,
                GAMEARRAY_READONLY|GAMEARRAY_INT16|GAMEARRAY_VARSIZE);
    Gv_NewArray("highlightsector", (void *)highlightsector, hlscnt_id,
                GAMEARRAY_READONLY|GAMEARRAY_INT16|GAMEARRAY_VARSIZE);

    Gv_NewArray("hsect", (void *)headspritesect, MAXSECTORS+1, GAMEARRAY_READONLY|GAMEARRAY_INT16);
    Gv_NewArray("psect", (void *)prevspritesect, MAXSPRITES, GAMEARRAY_READONLY|GAMEARRAY_INT16);
    Gv_NewArray("nsect", (void *)nextspritesect, MAXSPRITES, GAMEARRAY_READONLY|GAMEARRAY_INT16);
    Gv_NewArray("hstat", (void *)headspritestat, MAXSTATUS+1, GAMEARRAY_READONLY|GAMEARRAY_INT16);
    Gv_NewArray("pstat", (void *)prevspritestat, MAXSPRITES, GAMEARRAY_READONLY|GAMEARRAY_INT16);
    Gv_NewArray("nstat", (void *)nextspritestat, MAXSPRITES, GAMEARRAY_READONLY|GAMEARRAY_INT16);
#ifdef YAX_ENABLE
    Gv_NewArray("headsectbunchc", (void *)headsectbunch[0], YAX_MAXBUNCHES, GAMEARRAY_READONLY|GAMEARRAY_INT16);
    Gv_NewArray("nextsectbunchc", (void *)nextsectbunch[0], MAXSECTORS, GAMEARRAY_READONLY|GAMEARRAY_INT16);
    Gv_NewArray("headsectbunchf", (void *)headsectbunch[1], YAX_MAXBUNCHES, GAMEARRAY_READONLY|GAMEARRAY_INT16);
    Gv_NewArray("nextsectbunchf", (void *)nextsectbunch[1], MAXSECTORS, GAMEARRAY_READONLY|GAMEARRAY_INT16);
#endif
    Gv_NewArray("editorcolors", (void *)editorcolors, 256, GAMEARRAY_READONLY|GAMEARRAY_UINT8);
    Gv_NewArray("tilesizx", (void *)&tilesiz[0].x, MAXTILES, GAMEARRAY_STRIDE2|GAMEARRAY_READONLY|GAMEARRAY_INT16);
    Gv_NewArray("tilesizy", (void *)&tilesiz[0].y, MAXTILES, GAMEARRAY_STRIDE2|GAMEARRAY_READONLY|GAMEARRAY_INT16);
//    Gv_NewArray("picsiz", (void *)picsiz, MAXTILES, GAMEARRAY_READONLY|GAMEARRAY_OFCHAR);
    Gv_NewArray("picanm", (void *)picanm, MAXTILES, GAMEARRAY_READONLY|GAMEARRAY_INT32);

    Gv_NewArray("show2dsector", (void *)show2dsector, (MAXSECTORS+7)>>3, GAMEARRAY_READONLY|GAMEARRAY_UINT8);
    Gv_NewArray("show2dwall", (void *)show2dwall, (MAXWALLS+7)>>3, GAMEARRAY_READONLY|GAMEARRAY_UINT8);
    Gv_NewArray("show2dsprite", (void *)show2dsprite, (MAXSPRITES+7)>>3, GAMEARRAY_READONLY|GAMEARRAY_UINT8);

    Gv_NewArray("keystatus", (void *)keystatus, 256, GAMEARRAY_WARN|GAMEARRAY_UINT8);
    Gv_NewArray("alphakeys", (void *)alphakeys, sizeof(alphakeys), GAMEARRAY_READONLY|GAMEARRAY_UINT8);
    Gv_NewArray("numberkeys", (void *)numberkeys, sizeof(numberkeys), GAMEARRAY_READONLY|GAMEARRAY_UINT8);

    g_systemArrayCount = g_gameArrayCount;
}

void Gv_Init(void)
{
    // only call ONCE

    Gv_Clear();
    Gv_AddSystemVars();
}
