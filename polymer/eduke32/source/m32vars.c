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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include "m32script.h"
#include "m32def.h"
#include "osd.h"
#include "keys.h"

#define _m32vars_c_
#include "m32structures.c"

static void Gv_Clear(void)
{
    // only call this function ONCE...
    int32_t i=(MAXGAMEVARS-1);

    //AddLog("Gv_Clear");

    for (; i>=0; i--)
    {
        if (aGameVars[i].szLabel)
            Bfree(aGameVars[i].szLabel);
        aGameVars[i].szLabel = NULL;
        aGameVars[i].dwFlags = 0;

        if ((aGameVars[i].dwFlags & GAMEVAR_USER_MASK) && aGameVars[i].val.plValues)
        {
            Bfree(aGameVars[i].val.plValues);
            aGameVars[i].val.plValues = NULL;
        }
        aGameVars[i].val.lValue=0;
        aGameVars[i].dwFlags |= GAMEVAR_RESET;
        if (i >= MAXGAMEARRAYS)
            continue;
        if (aGameArrays[i].szLabel)
            Bfree(aGameArrays[i].szLabel);
        aGameArrays[i].szLabel = NULL;

        if (aGameArrays[i].vals)
            Bfree(aGameArrays[i].vals);
        aGameArrays[i].vals = NULL;
        aGameArrays[i].dwFlags |= GAMEARRAY_RESET;
    }
    g_gameVarCount = g_gameArrayCount = 0;
    hash_init(&h_gamevars);
    hash_init(&h_arrays);
    return;
}

int32_t Gv_NewArray(const char *pszLabel, void *arrayptr, intptr_t asize, uint32_t dwFlags)
{
    int32_t i;

    if (g_gameArrayCount >= MAXGAMEARRAYS)
    {
        g_numCompilerErrors++;
        C_ReportError(-1);
        initprintf("%s:%d: error: too many arrays! (max: %d)\n",g_szScriptFileName,g_lineNumber, MAXGAMEARRAYS);
        return 0;
    }

    if (Bstrlen(pszLabel) > (MAXARRAYLABEL-1))
    {
        g_numCompilerErrors++;
        C_ReportError(-1);
        initprintf("%s:%d: error: array name `%s' exceeds limit of %d characters.\n",g_szScriptFileName,g_lineNumber,pszLabel, MAXARRAYLABEL);
        return 0;
    }

    i = hash_find(&h_arrays, pszLabel);
    if (i>=0 && !(aGameArrays[i].dwFlags & GAMEARRAY_RESET))
    {
        // found it it's a duplicate in error

        if (aGameArrays[i].dwFlags&GAMEARRAY_TYPEMASK)
        {
            g_numCompilerWarnings++;
            C_ReportError(-1);
            initprintf("%s:%d: warning: didn't redefine system array `%s'.\n",g_szScriptFileName,g_lineNumber,pszLabel);
        }
//        C_ReportError(WARNING_DUPLICATEDEFINITION);
        return 0;
    }

    if (!(dwFlags&GAMEARRAY_VARSIZE) && (asize<=0 || asize>=65536))
    {
        g_numCompilerErrors++;
        C_ReportError(-1);
        initprintf("%s:%d: error: invalid array size %d. Must be between 1 and 65536\n",g_szScriptFileName,g_lineNumber,(int32_t)asize);
        return 0;
    }

    i = g_gameArrayCount;

    if (aGameArrays[i].szLabel == NULL)
        aGameArrays[i].szLabel = Bcalloc(MAXARRAYLABEL, sizeof(char));
    if (aGameArrays[i].szLabel != pszLabel)
        Bstrcpy(aGameArrays[i].szLabel, pszLabel);

    if (!(dwFlags & GAMEARRAY_TYPEMASK))
        aGameArrays[i].vals = Bcalloc(asize, sizeof(int32_t));
    else
        aGameArrays[i].vals = arrayptr;

    aGameArrays[i].size = asize;
    aGameArrays[i].dwFlags = dwFlags & ~GAMEARRAY_RESET;

    g_gameArrayCount++;
    hash_replace(&h_arrays, aGameArrays[i].szLabel, i);
    return 1;
}

int32_t Gv_NewVar(const char *pszLabel, intptr_t lValue, uint32_t dwFlags)
{
    int32_t i, j;

    //Bsprintf(g_szBuf,"Gv_NewVar(%s, %d, %X)",pszLabel, lValue, dwFlags);
    //AddLog(g_szBuf);

    if (g_gameVarCount >= MAXGAMEVARS)
    {
        g_numCompilerErrors++;
        C_ReportError(-1);
        initprintf("%s:%d: error: too many gamevars!\n",g_szScriptFileName,g_lineNumber);
        return 0;
    }

    if (Bstrlen(pszLabel) > (MAXVARLABEL-1))
    {
        g_numCompilerErrors++;
        C_ReportError(-1);
        initprintf("%s:%d: error: variable name `%s' exceeds limit of %d characters.\n",g_szScriptFileName,g_lineNumber,pszLabel, MAXVARLABEL);
        return 0;
    }

    i = hash_find(&h_gamevars,pszLabel);

    if (i >= 0 && !(aGameVars[i].dwFlags & GAMEVAR_RESET))
    {
        // found it...
        if (aGameVars[i].dwFlags & (GAMEVAR_INTPTR|GAMEVAR_SHORTPTR|GAMEVAR_CHARPTR))
        {
            C_ReportError(-1);
            initprintf("%s:%d: warning: cannot redefine internal gamevar `%s'.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
            return 0;
        }
        else if (!(aGameVars[i].dwFlags & GAMEVAR_DEFAULT) && !(aGameVars[i].dwFlags & GAMEVAR_SYSTEM))
        {
            // it's a duplicate in error
//            g_numCompilerWarnings++;
//            C_ReportError(WARNING_DUPLICATEDEFINITION);
            return 0;
        }
    }

    if (i == -1)
        i = g_gameVarCount;

    // Set values
    if ((aGameVars[i].dwFlags & GAMEVAR_SYSTEM) == 0)
    {
        if (aGameVars[i].szLabel == NULL)
            aGameVars[i].szLabel = Bcalloc(MAXVARLABEL, sizeof(uint8_t));
        if (aGameVars[i].szLabel != pszLabel)
            Bstrcpy(aGameVars[i].szLabel,pszLabel);
        aGameVars[i].dwFlags = dwFlags;

        if (aGameVars[i].dwFlags & GAMEVAR_USER_MASK)
        {
            // only free if not system
            if (aGameVars[i].val.plValues)
                Bfree(aGameVars[i].val.plValues);
            aGameVars[i].val.plValues = NULL;
        }
    }

    // if existing is system, they only get to change default value....
    aGameVars[i].lDefault = lValue;
    aGameVars[i].dwFlags &= ~GAMEVAR_RESET;

    if (i == g_gameVarCount)
    {
        // we're adding a new one.
        hash_add(&h_gamevars, aGameVars[i].szLabel, g_gameVarCount++);
    }

    if (aGameVars[i].dwFlags & GAMEVAR_PERBLOCK)
    {
        if (!aGameVars[i].val.plValues)
            aGameVars[i].val.plValues = Bcalloc(1+MAXEVENTS+g_stateCount, sizeof(int32_t));
        for (j=0; j<1+MAXEVENTS+g_stateCount; j++)
            aGameVars[i].val.plValues[j] = lValue;
    }
    else aGameVars[i].val.lValue = lValue;

    return 1;
}

int32_t __fastcall Gv_GetVarN(register int32_t id)  // 'N' for "no side-effects"... vars only!
{
    if (id == g_iThisActorID)
        return vm.g_i;

    if (id & (0xFFFFFFFF-(MAXGAMEVARS-1)))
    {
        OSD_Printf(CON_ERROR "Gv_GetVarN(): invalid var index %d\n",g_errorLineNum,keyw[g_tw],id);
        vm.flags |= VMFLAG_ERROR;
        return -1;
    }

    id &= (MAXGAMEVARS-1);

    switch (aGameVars[id].dwFlags &
            (GAMEVAR_USER_MASK|GAMEVAR_INTPTR|GAMEVAR_SHORTPTR|GAMEVAR_CHARPTR))
    {
    case 0:
        return aGameVars[id].val.lValue;
    case GAMEVAR_PERBLOCK:
        return aGameVars[id].val.plValues[vm.g_st];
    case GAMEVAR_INTPTR:
        return *((int32_t*)aGameVars[id].val.lValue);
    case GAMEVAR_SHORTPTR:
        return *((int16_t*)aGameVars[id].val.lValue);
    case GAMEVAR_CHARPTR:
        return *((uint8_t*)aGameVars[id].val.lValue);
    default:
        OSD_Printf(CON_ERROR "Gv_GetVarN(): WTF??\n",g_errorLineNum,keyw[g_tw]);
        vm.flags |= VMFLAG_ERROR;
        return -1;
    }
}

int32_t __fastcall Gv_GetVarX(register int32_t id)
{
    if (id == g_iThisActorID)
        return vm.g_i;

    if ((id & 0x0000FFFC) == MAXGAMEVARS)
    {
        switch (id&3)
        {
        case 0:
            return ((int16_t)(id>>16));
        case 1:
            return constants[(id>>16)&0xffff];
        case 2:
            return labelval[(id>>16)&0xffff];
        default:
            OSD_Printf(CON_ERROR "Gv_GetVarX() (constant): WTF??\n",g_errorLineNum,keyw[g_tw]);
            vm.flags |= VMFLAG_ERROR;
            return -1;
        }
    }

    {
        register int32_t negateResult = (id&(MAXGAMEVARS<<1))>>(LOG2MAXGV+1);

        if (id & (0xFFFFFFFF-(MAXGAMEVARS-1)))
        {
            if (id&(MAXGAMEVARS<<2)) // array
            {
                register int32_t index;
                int32_t siz;

                index = (int32_t)((id>>16)&0xffff);
                if (!(id&MAXGAMEVARS))
                    index = Gv_GetVarN(index);

                id &= (MAXGAMEARRAYS-1);

                if (aGameArrays[id].dwFlags & GAMEARRAY_VARSIZE)
                    siz = Gv_GetVarN(aGameArrays[id].size);
                else
                    siz = aGameArrays[id].size;

                if (index < 0 || index >= siz)
                {
                    OSD_Printf(CON_ERROR "Gv_GetVarX(): invalid array index (%s[%d])\n",g_errorLineNum,keyw[g_tw],aGameArrays[id].szLabel,index);
                    return -1;
                }

                switch (aGameArrays[id].dwFlags & GAMEARRAY_TYPEMASK)
                {
                case 0:
                case GAMEARRAY_OFINT:
                    return ((((int32_t *)aGameArrays[id].vals)[index] ^ -negateResult) + negateResult);
                case GAMEARRAY_OFSHORT:
                    return ((((int16_t *)aGameArrays[id].vals)[index] ^ -negateResult) + negateResult);
                case GAMEARRAY_OFCHAR:
                    return ((((uint8_t *)aGameArrays[id].vals)[index] ^ -negateResult) + negateResult);
                default:
                    OSD_Printf(CON_ERROR "Gv_GetVarX() (array): WTF??\n",g_errorLineNum,keyw[g_tw]);
                    vm.flags |= VMFLAG_ERROR;
                    return -1;
                }
            }

            if (id&(MAXGAMEVARS<<3)) // struct shortcut vars
            {
                register int32_t index, memberid;

                index = (id>>16)&0x7fff;
                if (!(id&MAXGAMEVARS))
                    index = Gv_GetVarN(index);

                memberid = (id>>2)&31;

                switch (id&3)
                {
                case 0: //if (id == g_iSpriteVarID)
                    return ((VM_AccessSprite(0, index, memberid, 0) ^ -negateResult) + negateResult);
                case 1: //else if (id == g_iSectorVarID)
//                    if (index == vm.g_i) index = sprite[vm.g_i].sectnum;
                    return ((VM_AccessSector(0, index, memberid, 0) ^ -negateResult) + negateResult);
                case 2: //else if (id == g_iWallVarID)
                    return ((VM_AccessWall(0, index, memberid, 0) ^ -negateResult) + negateResult);
                case 3:
                    return ((VM_AccessTsprite(0, index, memberid, 0) ^ -negateResult) + negateResult);
//                default:
//                    OSD_Printf(CON_ERROR "Gv_GetVarX() (special): WTF??\n",g_errorLineNum,keyw[g_tw]);
//                    return -1;
                }
            }

            id &= (MAXGAMEVARS-1);

            if (!negateResult)
            {
                OSD_Printf(CON_ERROR "Gv_GetVarX(): invalid gamevar ID (%d)\n",g_errorLineNum,keyw[g_tw],id);
                vm.flags |= VMFLAG_ERROR;
                return -1;
            }
        }

        switch (aGameVars[id].dwFlags &
                (GAMEVAR_USER_MASK|GAMEVAR_INTPTR|GAMEVAR_SHORTPTR|GAMEVAR_CHARPTR))
        {
        case 0:
            return ((aGameVars[id].val.lValue ^ -negateResult) + negateResult);
        case GAMEVAR_PERBLOCK:
            return ((aGameVars[id].val.plValues[vm.g_st] ^ -negateResult) + negateResult);
        case GAMEVAR_INTPTR:
            return ((*((int32_t*)aGameVars[id].val.lValue) ^ -negateResult) + negateResult);
        case GAMEVAR_SHORTPTR:
            return ((*((int16_t*)aGameVars[id].val.lValue) ^ -negateResult) + negateResult);
        case GAMEVAR_CHARPTR:
            return ((*((uint8_t*)aGameVars[id].val.lValue) ^ -negateResult) + negateResult);
        default:
            OSD_Printf(CON_ERROR "Gv_GetVarX(): WTF??\n",g_errorLineNum,keyw[g_tw]);
            vm.flags |= VMFLAG_ERROR;
            return -1;
        }
    }
}

void __fastcall Gv_SetVarX(register int32_t id, register int32_t lValue)
{
    if (id & (0xFFFFFFFF-(MAXGAMEVARS-1)))
    {
        if (id&(MAXGAMEVARS<<2)) // array
        {
            register int32_t index;
            int32_t siz;

            index = (id>>16)&0xffff;
            if (!(id&MAXGAMEVARS))
                index = Gv_GetVarN(index);

            id &= (MAXGAMEARRAYS-1);

            if (aGameArrays[id].dwFlags & GAMEARRAY_VARSIZE)
                siz = Gv_GetVarN(aGameArrays[id].size);
            else siz = aGameArrays[id].size;

            if (index < 0 || index >= siz)
            {
                OSD_Printf(CON_ERROR "Gv_SetVarX(): invalid array index (%s[%d])\n",g_errorLineNum,keyw[g_tw],aGameArrays[id].szLabel,index);
                vm.flags |= VMFLAG_ERROR;
                return;
            }

            switch (aGameArrays[id].dwFlags & GAMEARRAY_TYPEMASK)
            {
            case 0:
            case GAMEARRAY_OFINT:
                ((int32_t *)aGameArrays[id].vals)[index] = lValue;
                return;
            case GAMEARRAY_OFSHORT:
                ((int16_t *)aGameArrays[id].vals)[index] = (int16_t)lValue;
                return;
            case GAMEARRAY_OFCHAR:
                ((uint8_t *)aGameArrays[id].vals)[index] = (uint8_t)lValue;
                return;
            default:
                OSD_Printf(CON_ERROR "Gv_SetVarX() (array): WTF??\n",g_errorLineNum,keyw[g_tw]);
                vm.flags |= VMFLAG_ERROR;
                return;
            }
            return;
        }

        if (id&(MAXGAMEVARS<<3)) // struct shortcut vars
        {
            register int32_t index, memberid;

            index = (id>>16)&0x7fff;
            if (!(id&MAXGAMEVARS))
                index = Gv_GetVarN(index);

            memberid = (id>>2)&31;

            switch (id&3)
            {
            case 0: //if (id == g_iSpriteVarID)
                VM_AccessSprite(1, index, memberid, lValue);
                return;
            case 1: //else if (id == g_iSectorVarID)
//                    if (index == vm.g_i) index = sprite[vm.g_i].sectnum;
                VM_AccessSector(1, index, memberid, lValue);
                return;
            case 2: //else if (id == g_iWallVarID)
                VM_AccessWall(1, index, memberid, lValue);
                return;
            case 3:
                VM_AccessTsprite(1, index, memberid, lValue);
                return;
//                default:
//                    OSD_Printf(CON_ERROR "Gv_SetVarX(): WTF??\n",g_errorLineNum,keyw[g_tw]);
//                    return;
            }
        }

        OSD_Printf(CON_ERROR "Gv_SetVarX(): invalid gamevar ID (%d)\n",g_errorLineNum,keyw[g_tw],id);
        vm.flags |= VMFLAG_ERROR;
        return;
    }

    switch (aGameVars[id].dwFlags &
            (GAMEVAR_USER_MASK|GAMEVAR_INTPTR|GAMEVAR_SHORTPTR|GAMEVAR_CHARPTR))
    {
    case 0:
        aGameVars[id].val.lValue=lValue;
        return;
    case GAMEVAR_PERBLOCK:
        aGameVars[id].val.plValues[vm.g_st] = lValue;
        return;
    case GAMEVAR_INTPTR:
        *((int32_t*)aGameVars[id].val.lValue)=(int32_t)lValue;
        return;
    case GAMEVAR_SHORTPTR:
        *((int16_t*)aGameVars[id].val.lValue)=(int16_t)lValue;
        return;
    case GAMEVAR_CHARPTR:
        *((uint8_t*)aGameVars[id].val.lValue)=(uint8_t)lValue;
        return;
    default:
        OSD_Printf(CON_ERROR "Gv_SetVarX(): WTF??\n",g_errorLineNum,keyw[g_tw]);
        vm.flags |= VMFLAG_ERROR;
        return;
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

    //AddLog("Gv_AddSystemVars");

    // special vars for struct access; must be at top and in this order
    g_iSpriteVarID = g_gameVarCount;
    Gv_NewVar("sprite", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    g_iSectorVarID = g_gameVarCount;
    Gv_NewVar("sector", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    g_iWallVarID = g_gameVarCount;
    Gv_NewVar("wall", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("tsprite", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);

    g_iReturnVarID = g_gameVarCount;
    Gv_NewVar("RETURN", (intptr_t)&g_iReturnVar, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    g_iLoTagID = g_gameVarCount;
    Gv_NewVar("LOTAG", 0, GAMEVAR_SYSTEM);
    g_iHiTagID = g_gameVarCount;
    Gv_NewVar("HITAG", 0, GAMEVAR_SYSTEM);
    g_iTextureID = g_gameVarCount;
    Gv_NewVar("TEXTURE", 0, GAMEVAR_SYSTEM);
    g_iThisActorID = g_gameVarCount;
    Gv_NewVar("I", 0, GAMEVAR_READONLY | GAMEVAR_SYSTEM);  // THISACTOR

    Gv_NewVar("xdim",(intptr_t)&xdim, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("ydim",(intptr_t)&ydim, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("windowx1",(intptr_t)&windowx1, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("windowx2",(intptr_t)&windowx2, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("windowy1",(intptr_t)&windowy1, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("windowy2",(intptr_t)&windowy2, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("totalclock",(intptr_t)&totalclock, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);

    Gv_NewVar("viewingrange",(intptr_t)&viewingrange, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("yxaspect",(intptr_t)&yxaspect, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);

///    Gv_NewVar("framerate",(intptr_t)&g_currentFrameRate, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
///    Gv_NewVar("display_mirror",(intptr_t)&display_mirror, GAMEVAR_SYSTEM | GAMEVAR_CHARPTR);

    Gv_NewVar("randomseed",(intptr_t)&randomseed, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);

    Gv_NewVar("numwalls",(intptr_t)&numwalls, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_READONLY);
    Gv_NewVar("numsectors",(intptr_t)&numsectors, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_READONLY);
    Gv_NewVar("numsprites",(intptr_t)&numsprites, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("numtiles",(intptr_t)&numtiles, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);

#ifdef POLYMOST
    Gv_NewVar("rendmode",(intptr_t)&rendmode, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
#endif

    // current position
    Gv_NewVar("posx",(intptr_t)&pos.x, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("posy",(intptr_t)&pos.y, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("posz",(intptr_t)&pos.z, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("ang",(intptr_t)&ang, GAMEVAR_SHORTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("horiz",(intptr_t)&horiz, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("cursectnum",(intptr_t)&cursectnum, GAMEVAR_READONLY | GAMEVAR_SHORTPTR | GAMEVAR_SYSTEM);

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
    Gv_NewVar("startposx",(intptr_t)&startposx, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("startposy",(intptr_t)&startposy, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("startposz",(intptr_t)&startposz, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("startang",(intptr_t)&startang, GAMEVAR_READONLY | GAMEVAR_SHORTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("startsectnum",(intptr_t)&startsectnum, GAMEVAR_READONLY | GAMEVAR_SHORTPTR | GAMEVAR_SYSTEM);

    Gv_NewVar("mousxplc",(intptr_t)&mousxplc, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("mousyplc",(intptr_t)&mousyplc, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);

    Gv_NewVar("zoom",(intptr_t)&zoom, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("drawlinepat",(intptr_t)&m32_drawlinepat, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("halfxdim16", (intptr_t)&halfxdim16, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("midydim16", (intptr_t)&midydim16, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("ydim16",(intptr_t)&ydim16, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);

    Gv_NewVar("SV1",(intptr_t)&m32_sortvar1, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("SV2",(intptr_t)&m32_sortvar2, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("spritesortcnt",(intptr_t)&spritesortcnt, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);

    g_systemVarCount = g_gameVarCount;


    Gv_NewArray("highlight", (void *)highlight, hlcnt_id,
                GAMEARRAY_READONLY|GAMEARRAY_OFSHORT|GAMEARRAY_VARSIZE);
    Gv_NewArray("highlightsector", (void *)highlightsector, hlscnt_id,
                GAMEARRAY_READONLY|GAMEARRAY_OFSHORT|GAMEARRAY_VARSIZE);

    Gv_NewArray("hsect", (void *)headspritesect, MAXSECTORS+1, GAMEARRAY_READONLY|GAMEARRAY_OFSHORT);
    Gv_NewArray("nsect", (void *)prevspritesect, MAXSPRITES, GAMEARRAY_READONLY|GAMEARRAY_OFSHORT);
    Gv_NewArray("psect", (void *)nextspritesect, MAXSPRITES, GAMEARRAY_READONLY|GAMEARRAY_OFSHORT);
    Gv_NewArray("hstat", (void *)headspritestat, MAXSTATUS+1, GAMEARRAY_READONLY|GAMEARRAY_OFSHORT);
    Gv_NewArray("nstat", (void *)prevspritestat, MAXSPRITES, GAMEARRAY_READONLY|GAMEARRAY_OFSHORT);
    Gv_NewArray("pstat", (void *)nextspritestat, MAXSPRITES, GAMEARRAY_READONLY|GAMEARRAY_OFSHORT);

    Gv_NewArray("tilesizx", (void *)tilesizx, MAXTILES, GAMEARRAY_READONLY|GAMEARRAY_OFSHORT);
    Gv_NewArray("tilesizy", (void *)tilesizy, MAXTILES, GAMEARRAY_READONLY|GAMEARRAY_OFSHORT);

    Gv_NewArray("show2dsector", (void *)show2dsector, (MAXSECTORS+7)>>3, GAMEARRAY_READONLY|GAMEARRAY_OFCHAR);
    Gv_NewArray("show2dwall", (void *)show2dwall, (MAXWALLS+7)>>3, GAMEARRAY_READONLY|GAMEARRAY_OFCHAR);
    Gv_NewArray("show2dsprite", (void *)show2dsprite, (MAXSPRITES+7)>>3, GAMEARRAY_READONLY|GAMEARRAY_OFCHAR);

    Gv_NewArray("keystatus", (void *)keystatus, 256, GAMEARRAY_READONLY|GAMEARRAY_OFCHAR);
    Gv_NewArray("alphakeys", (void *)alphakeys, sizeof(alphakeys), GAMEARRAY_READONLY|GAMEARRAY_OFCHAR);
    Gv_NewArray("numberkeys", (void *)numberkeys, sizeof(numberkeys), GAMEARRAY_READONLY|GAMEARRAY_OFCHAR);

    g_systemArrayCount = g_gameArrayCount;
}

void Gv_Init(void)
{
    // only call ONCE

    //  initprintf("Initializing game variables\n");
    //AddLog("Gv_Init");

    Gv_Clear();
    Gv_AddSystemVars();
}
