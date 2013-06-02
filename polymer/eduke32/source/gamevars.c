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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include "duke3d.h"
#include "common_game.h"
#include "gamevars.h"
#include "gamedef.h"
#include "osd.h"
#include "savegame.h"

#define _gamevars_c_

#if !defined LUNATIC
# include "gamestructures.c"

extern int32_t OSD_errors;

static void Gv_Free(void) /* called from Gv_ReadSave() and Gv_ResetVars() */
{
    // call this function as many times as needed.
    int32_t i;

    for (i=MAXGAMEVARS-1; i>=0; i--)
    {
        if ((aGameVars[i].dwFlags & GAMEVAR_USER_MASK) && aGameVars[i].val.plValues)
        {
            Bfree(aGameVars[i].val.plValues);
            aGameVars[i].val.plValues=NULL;
        }

        aGameVars[i].dwFlags |= GAMEVAR_RESET;

        if (i >= MAXGAMEARRAYS)
            continue;

        if ((aGameArrays[i].dwFlags & GAMEARRAY_NORMAL) && aGameArrays[i].plValues)
        {
            Bfree(aGameArrays[i].plValues);
            aGameArrays[i].plValues=NULL;
        }

        aGameArrays[i].dwFlags |= GAMEARRAY_RESET;
    }

    g_gameVarCount = g_gameArrayCount = 0;
    hash_init(&h_gamevars);
    hash_init(&h_arrays);
}

static void Gv_Clear(void)
{
    // only call this function ONCE...
    int32_t i;

    Gv_Free();

    // Now, only do work that Gv_Free() hasn't done.
    for (i=MAXGAMEVARS-1; i>=0; i--)
    {
        Bfree(aGameVars[i].szLabel);
        aGameVars[i].szLabel=NULL;

        aGameVars[i].val.lValue=0;

        if (i >= MAXGAMEARRAYS)
            continue;

        Bfree(aGameArrays[i].szLabel);
        aGameArrays[i].szLabel=NULL;
    }
}

int32_t Gv_ReadSave(int32_t fil, int32_t newbehav)
{
    int32_t i, j;
    char savedstate[MAXVOLUMES*MAXLEVELS];
    char tbuf[12];

    if (newbehav)
    {
        if (kread(fil, tbuf, 12)!=12) goto corrupt;
        if (Bmemcmp(tbuf, "BEG: EDuke32", 12)) { OSD_Printf("BEG ERR\n"); return 2; }
    }

    Bmemset(&savedstate,0,sizeof(savedstate));

    //     AddLog("Reading gamevars from savegame");

    Gv_Free(); // nuke 'em from orbit, it's the only way to be sure...

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);

    if (kdfread(&g_gameVarCount,sizeof(g_gameVarCount),1,fil) != 1) goto corrupt;
    for (i=0; i<g_gameVarCount; i++)
    {
        if (kdfread(&(aGameVars[i]),sizeof(gamevar_t),1,fil) != 1) goto corrupt;
        aGameVars[i].szLabel = (char *)Bcalloc(MAXVARLABEL,sizeof(uint8_t));
        if (kdfread(aGameVars[i].szLabel,sizeof(uint8_t) * MAXVARLABEL, 1, fil) != 1) goto corrupt;
        hash_add(&h_gamevars, aGameVars[i].szLabel,i, 1);

        if (aGameVars[i].dwFlags & GAMEVAR_PERPLAYER)
        {
            aGameVars[i].val.plValues = (intptr_t*)Bcalloc(MAXPLAYERS,sizeof(intptr_t));
            if (kdfread(aGameVars[i].val.plValues,sizeof(intptr_t) * MAXPLAYERS, 1, fil) != 1) goto corrupt;
        }
        else if (aGameVars[i].dwFlags & GAMEVAR_PERACTOR)
        {
            aGameVars[i].val.plValues = (intptr_t*)Bcalloc(MAXSPRITES,sizeof(intptr_t));
            if (kdfread(&aGameVars[i].val.plValues[0],sizeof(intptr_t), MAXSPRITES, fil) != MAXSPRITES) goto corrupt;
        }
    }
    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);

    Gv_InitWeaponPointers();

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    Gv_RefreshPointers();

    if (kdfread(&g_gameArrayCount,sizeof(g_gameArrayCount),1,fil) != 1) goto corrupt;
    for (i=0; i<g_gameArrayCount; i++)
    {
        if (aGameArrays[i].dwFlags&GAMEARRAY_READONLY)
            continue;

        // read for .size and .dwFlags (the rest are pointers):
        if (kdfread(&aGameArrays[i],sizeof(gamearray_t),1,fil) != 1) goto corrupt;

        aGameArrays[i].szLabel = (char *)Bcalloc(MAXARRAYLABEL,sizeof(uint8_t));
        if (kdfread(aGameArrays[i].szLabel,sizeof(uint8_t) * MAXARRAYLABEL, 1, fil) != 1) goto corrupt;
        hash_add(&h_arrays, aGameArrays[i].szLabel, i, 1);

        aGameArrays[i].plValues = (intptr_t *)Bcalloc(aGameArrays[i].size, GAR_ELTSZ);
        if (kdfread(aGameArrays[i].plValues, GAR_ELTSZ * aGameArrays[i].size, 1, fil) < 1) goto corrupt;
    }

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    if (kdfread(apScriptGameEvent,sizeof(apScriptGameEvent),1,fil) != 1) goto corrupt;
    G_Util_PtrToIdx(apScriptGameEvent, MAXGAMEEVENTS, script, P2I_BACK_NON0);

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);

    if (kdfread(&savedstate[0],sizeof(savedstate),1,fil) != 1) goto corrupt;

    for (i=0; i<(MAXVOLUMES*MAXLEVELS); i++)
    {
        if (savedstate[i])
        {
            if (MapInfo[i].savedstate == NULL)
                MapInfo[i].savedstate = (mapstate_t *)Bcalloc(1,sizeof(mapstate_t));
            if (kdfread(MapInfo[i].savedstate,sizeof(mapstate_t),1,fil) != sizeof(mapstate_t)) goto corrupt;
            for (j=0; j<g_gameVarCount; j++)
            {
                if (aGameVars[j].dwFlags & GAMEVAR_NORESET) continue;
                if (aGameVars[j].dwFlags & GAMEVAR_PERPLAYER)
                {
//                    if (!MapInfo[i].savedstate->vars[j])
                    MapInfo[i].savedstate->vars[j] = (intptr_t *)Bcalloc(MAXPLAYERS,sizeof(intptr_t));
                    if (kdfread(&MapInfo[i].savedstate->vars[j][0],sizeof(intptr_t) * MAXPLAYERS, 1, fil) != 1) goto corrupt;
                }
                else if (aGameVars[j].dwFlags & GAMEVAR_PERACTOR)
                {
//                    if (!MapInfo[i].savedstate->vars[j])
                    MapInfo[i].savedstate->vars[j] = (intptr_t *)Bcalloc(MAXSPRITES,sizeof(intptr_t));
                    if (kdfread(&MapInfo[i].savedstate->vars[j][0],sizeof(intptr_t), MAXSPRITES, fil) != MAXSPRITES) goto corrupt;
                }
            }
        }
        else
        {
            G_FreeMapState(i);
        }
    }

    if (!newbehav)
    {
        intptr_t l;

        if (kdfread(&l,sizeof(l),1,fil) != 1) goto corrupt;
        if (kdfread(g_szBuf,l,1,fil) != 1) goto corrupt;
        g_szBuf[l]=0;
        OSD_Printf("%s\n",g_szBuf);
    }
    else
    {
        if (kread(fil, tbuf, 12)!=12) goto corrupt;
        if (Bmemcmp(tbuf, "EOF: EDuke32", 12)) { OSD_Printf("EOF ERR\n"); return 2; }
    }

# if 0
    {
        FILE *fp;
        AddLog("Dumping Vars...");
        fp=fopen("xxx.txt","w");
        if (fp)
        {
            Gv_DumpValues(fp);
            fclose(fp);
        }
        AddLog("Done Dumping...");
    }
# endif
    return(0);
corrupt:
    return(1);
}

void Gv_WriteSave(FILE *fil, int32_t newbehav)
{
    int32_t i, j;
    char savedstate[MAXVOLUMES*MAXLEVELS];

    Bmemset(&savedstate,0,sizeof(savedstate));

    //   AddLog("Saving Game Vars to File");
    if (newbehav)
        fwrite("BEG: EDuke32", 12, 1, fil);

    dfwrite(&g_gameVarCount,sizeof(g_gameVarCount),1,fil);

    for (i=0; i<g_gameVarCount; i++)
    {
        dfwrite(&(aGameVars[i]),sizeof(gamevar_t),1,fil);
        dfwrite(aGameVars[i].szLabel,sizeof(uint8_t) * MAXVARLABEL, 1, fil);

        if (aGameVars[i].dwFlags & GAMEVAR_PERPLAYER)
        {
            //Bsprintf(g_szBuf,"Writing value array for %s (%d)",aGameVars[i].szLabel,sizeof(int32_t) * MAXPLAYERS);
            //AddLog(g_szBuf);
            dfwrite(aGameVars[i].val.plValues,sizeof(intptr_t) * MAXPLAYERS, 1, fil);
        }
        else if (aGameVars[i].dwFlags & GAMEVAR_PERACTOR)
        {
            //Bsprintf(g_szBuf,"Writing value array for %s (%d)",aGameVars[i].szLabel,sizeof(int32_t) * MAXSPRITES);
            //AddLog(g_szBuf);
            dfwrite(&aGameVars[i].val.plValues[0],sizeof(intptr_t), MAXSPRITES, fil);
        }
    }

    dfwrite(&g_gameArrayCount,sizeof(g_gameArrayCount),1,fil);

    for (i=0; i<g_gameArrayCount; i++)
    {
        if (aGameArrays[i].dwFlags&GAMEARRAY_READONLY)
            continue;

        // write for .size and .dwFlags (the rest are pointers):
        dfwrite(&aGameArrays[i],sizeof(gamearray_t),1,fil);

        dfwrite(aGameArrays[i].szLabel,sizeof(uint8_t) * MAXARRAYLABEL, 1, fil);
        dfwrite(aGameArrays[i].plValues, GAR_ELTSZ * aGameArrays[i].size, 1, fil);
    }

    G_Util_PtrToIdx(apScriptGameEvent, MAXGAMEEVENTS, script, P2I_FWD_NON0);
    dfwrite(apScriptGameEvent,sizeof(apScriptGameEvent),1,fil);
    G_Util_PtrToIdx(apScriptGameEvent, MAXGAMEEVENTS, script, P2I_BACK_NON0);

    for (i=0; i<(MAXVOLUMES*MAXLEVELS); i++)
        if (MapInfo[i].savedstate != NULL)
            savedstate[i] = 1;

    dfwrite(&savedstate[0],sizeof(savedstate),1,fil);

    for (i=0; i<(MAXVOLUMES*MAXLEVELS); i++)
        if (MapInfo[i].savedstate)
        {
            dfwrite(MapInfo[i].savedstate,sizeof(mapstate_t),1,fil);
            for (j=0; j<g_gameVarCount; j++)
            {
                if (aGameVars[j].dwFlags & GAMEVAR_NORESET) continue;
                if (aGameVars[j].dwFlags & GAMEVAR_PERPLAYER)
                {
                    dfwrite(&MapInfo[i].savedstate->vars[j][0],sizeof(intptr_t) * MAXPLAYERS, 1, fil);
                }
                else if (aGameVars[j].dwFlags & GAMEVAR_PERACTOR)
                {
                    dfwrite(&MapInfo[i].savedstate->vars[j][0],sizeof(intptr_t), MAXSPRITES, fil);
                }
            }
        }

    if (!newbehav)
    {
        intptr_t l;

        Bsprintf(g_szBuf,"EOF: EDuke32");
        l=Bstrlen(g_szBuf);
        dfwrite(&l,sizeof(l),1,fil);
        dfwrite(g_szBuf,l,1,fil);
    }
    else
        fwrite("EOF: EDuke32", 12, 1, fil);
}

void Gv_DumpValues(void)
{
    int32_t i;

    OSD_Printf("// Current Game Definitions\n\n");

    for (i=0; i<g_gameVarCount; i++)
    {
        if (aGameVars[i].dwFlags & (GAMEVAR_SECRET))
            continue; // do nothing...

        OSD_Printf("gamevar %s ",aGameVars[i].szLabel);

        if (aGameVars[i].dwFlags & (GAMEVAR_INTPTR))
            OSD_Printf("%d",*((int32_t *)aGameVars[i].val.lValue));
        else if (aGameVars[i].dwFlags & (GAMEVAR_SHORTPTR))
            OSD_Printf("%d",*((int16_t *)aGameVars[i].val.lValue));
        else if (aGameVars[i].dwFlags & (GAMEVAR_CHARPTR))
            OSD_Printf("%d",*((char *)aGameVars[i].val.lValue));
        else
            OSD_Printf("%" PRIdPTR "",aGameVars[i].val.lValue);

        if (aGameVars[i].dwFlags & (GAMEVAR_PERPLAYER))
            OSD_Printf(" GAMEVAR_PERPLAYER");
        else if (aGameVars[i].dwFlags & (GAMEVAR_PERACTOR))
            OSD_Printf(" GAMEVAR_PERACTOR");
        else
            OSD_Printf(" %"PRIdPTR,aGameVars[i].dwFlags/* & (GAMEVAR_USER_MASK)*/);

        OSD_Printf(" // ");
        if (aGameVars[i].dwFlags & (GAMEVAR_SYSTEM))
            OSD_Printf(" (system)");
        if (aGameVars[i].dwFlags & (GAMEVAR_PTR_MASK))
            OSD_Printf(" (pointer)");
        if (aGameVars[i].dwFlags & (GAMEVAR_READONLY))
            OSD_Printf(" (read only)");
        if (aGameVars[i].dwFlags & (GAMEVAR_SPECIAL))
            OSD_Printf(" (special)");
        OSD_Printf("\n");
    }
    OSD_Printf("\n// end of game definitions\n");
}

// XXX: This function is very strange.
void Gv_ResetVars(void) /* this is called during a new game and nowhere else */
{
    int32_t i;

    Gv_Free();
    OSD_errors=0;

    for (i=0; i<MAXGAMEVARS; i++)
    {
        if (aGameVars[i].szLabel != NULL)
            Gv_NewVar(aGameVars[i].szLabel,
                      aGameVars[i].dwFlags & GAMEVAR_NODEFAULT ? aGameVars[i].val.lValue : aGameVars[i].lDefault,
                      aGameVars[i].dwFlags);
    }

    for (i=0; i<MAXGAMEARRAYS; i++)
    {
        if (aGameArrays[i].szLabel != NULL && (aGameArrays[i].dwFlags & GAMEARRAY_RESET))
            Gv_NewArray(aGameArrays[i].szLabel,aGameArrays[i].plValues,aGameArrays[i].size,aGameArrays[i].dwFlags);
    }
}

int32_t Gv_NewArray(const char *pszLabel, void *arrayptr, intptr_t asize, uint32_t dwFlags)
{
    int32_t i;

    if (g_gameArrayCount >= MAXGAMEARRAYS)
    {
        g_numCompilerErrors++;
        C_ReportError(-1);
        initprintf("%s:%d: error: too many arrays!\n",g_szScriptFileName,g_lineNumber);
        return 0;
    }

    if (Bstrlen(pszLabel) > (MAXARRAYLABEL-1))
    {
        g_numCompilerErrors++;
        C_ReportError(-1);
        initprintf("%s:%d: error: array name `%s' exceeds limit of %d characters.\n",g_szScriptFileName,g_lineNumber,pszLabel, MAXARRAYLABEL);
        return 0;
    }
    i = hash_find(&h_arrays,pszLabel);
    if (i >=0 && !(aGameArrays[i].dwFlags & GAMEARRAY_RESET))
    {
        // found it it's a duplicate in error

        g_numCompilerWarnings++;

        if (aGameArrays[i].dwFlags&GAMEARRAY_TYPE_MASK)
        {
            C_ReportError(-1);
            initprintf("ignored redefining system array `%s'.", pszLabel);
        }
        else
            C_ReportError(WARNING_DUPLICATEDEFINITION);

        return 0;
    }

    i = g_gameArrayCount;

    if (aGameArrays[i].szLabel == NULL)
        aGameArrays[i].szLabel=(char *)Bcalloc(MAXVARLABEL,sizeof(uint8_t));
    if (aGameArrays[i].szLabel != pszLabel)
        Bstrcpy(aGameArrays[i].szLabel,pszLabel);

    if (!(dwFlags & GAMEARRAY_TYPE_MASK))
        aGameArrays[i].plValues=(intptr_t *)Bcalloc(asize,GAR_ELTSZ);
    else
        aGameArrays[i].plValues=(intptr_t *)arrayptr;

    aGameArrays[i].size=asize;
    aGameArrays[i].dwFlags = dwFlags & ~GAMEARRAY_RESET;

    g_gameArrayCount++;
    hash_add(&h_arrays, aGameArrays[i].szLabel, i, 1);

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
        if (aGameVars[i].dwFlags & (GAMEVAR_PTR_MASK))
        {
            C_ReportError(-1);
            initprintf("%s:%d: warning: cannot redefine internal gamevar `%s'.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
            return 0;
        }
        else if (!(aGameVars[i].dwFlags & GAMEVAR_SYSTEM))
        {
            // it's a duplicate in error
            g_numCompilerWarnings++;
            C_ReportError(WARNING_DUPLICATEDEFINITION);
            return 0;
        }
    }

    if (i == -1)
        i = g_gameVarCount;

    // If it's a user gamevar...
    if ((aGameVars[i].dwFlags & GAMEVAR_SYSTEM) == 0)
    {
        // Allocate and set its label
        if (aGameVars[i].szLabel == NULL)
            aGameVars[i].szLabel = (char *)Bcalloc(MAXVARLABEL,sizeof(uint8_t));
        if (aGameVars[i].szLabel != pszLabel)
            Bstrcpy(aGameVars[i].szLabel,pszLabel);

        // and the flags
        aGameVars[i].dwFlags=dwFlags;

        if (aGameVars[i].dwFlags & GAMEVAR_USER_MASK)
        {
            // only free if per-{actor,player}
            Bfree(aGameVars[i].val.plValues);
            aGameVars[i].val.plValues=NULL;
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

    // Set initial values. (Or, override values for system gamevars.)
    if (aGameVars[i].dwFlags & GAMEVAR_PERPLAYER)
    {
        if (!aGameVars[i].val.plValues)
            aGameVars[i].val.plValues = (intptr_t *)Bcalloc(MAXPLAYERS,sizeof(intptr_t));
        for (j=MAXPLAYERS-1; j>=0; j--)
            aGameVars[i].val.plValues[j]=lValue;
    }
    else if (aGameVars[i].dwFlags & GAMEVAR_PERACTOR)
    {
        if (!aGameVars[i].val.plValues)
            aGameVars[i].val.plValues = (intptr_t *)Bcalloc(MAXSPRITES,sizeof(intptr_t));
        for (j=MAXSPRITES-1; j>=0; j--)
            aGameVars[i].val.plValues[j]=lValue;
    }
    else aGameVars[i].val.lValue = lValue;

    return 1;
}

void __fastcall A_ResetVars(register int32_t iActor)
{
    register int32_t i=(MAXGAMEVARS-1);
    do
    {
        if ((aGameVars[i].dwFlags & (GAMEVAR_PERACTOR|GAMEVAR_NODEFAULT)) == GAMEVAR_PERACTOR)
            aGameVars[i].val.plValues[iActor]=aGameVars[i].lDefault;
    }
    while (i--);
}

static int32_t Gv_GetVarIndex(const char *szGameLabel)
{
    int32_t i = hash_find(&h_gamevars,szGameLabel);
    if (i == -1)
    {
        OSD_Printf(OSD_ERROR "Gv_GetVarIndex(): INTERNAL ERROR: couldn't find gamevar %s!\n",szGameLabel);
        return 0;
    }
    return i;
}

int32_t __fastcall Gv_GetVar(register int32_t id, register int32_t iActor, register int32_t iPlayer)
{
    if (id == g_iThisActorID)
        return iActor;

    if (id == MAXGAMEVARS)
        return(*insptr++);

    {
        register intptr_t negateResult = id&(MAXGAMEVARS<<1);

        if (id >= g_gameVarCount)
        {
            if (id&(MAXGAMEVARS<<2)) // array
            {
                register int32_t index=Gv_GetVar(*insptr++,iActor,iPlayer);

                id &= (MAXGAMEVARS-1);// ~((MAXGAMEVARS<<2)|(MAXGAMEVARS<<1));

                if ((unsigned)index >= (unsigned)aGameArrays[id].size)
                {
                    iActor = index;
                    goto badindex;
                }

                return ((aGameArrays[id].plValues[index] ^ -negateResult) + negateResult);
            }

            if (id&(MAXGAMEVARS<<3)) // struct shortcut vars
            {
                register int32_t index=Gv_GetVar(*insptr++, iActor, iPlayer);

                switch ((id&(MAXGAMEVARS-1)) - g_iSpriteVarID)
                {
                case 0: //if (id == g_iSpriteVarID)
                {
                    int32_t parm2 = 0, label = *insptr++;

                    /*OSD_Printf("%d %d %d\n",__LINE__,index,label);*/
                    if (ActorLabels[label].flags & LABEL_HASPARM2)
                        parm2 = Gv_GetVar(*insptr++, iActor, iPlayer);

                    if ((unsigned)index >= MAXSPRITES)
                    {
                        iPlayer = index;
                        goto badsprite;
                    }

                    return ((VM_AccessSpriteX(index, label, parm2) ^ -negateResult) + negateResult);
                }
                case 3: //else if (id == g_iPlayerVarID)
                {
                    int32_t parm2 = 0, label = *insptr++;

                    if (PlayerLabels[label].flags & LABEL_HASPARM2)
                        parm2 = Gv_GetVar(*insptr++, iActor, iPlayer);

                    if (index == vm.g_i) index = vm.g_p;

                    if ((unsigned)index >= MAXPLAYERS)
                    {
                        iPlayer = index;
                        goto badplayer;
                    }

                    return ((VM_AccessPlayerX(index, label, parm2) ^ -negateResult) + negateResult);
                }
                case 4: //else if (id == g_iActorVarID)
                    return ((Gv_GetVar(*insptr++, index, iPlayer) ^ -negateResult) + negateResult);
                case 1: //else if (id == g_iSectorVarID)
                    if (index == vm.g_i) index = sprite[vm.g_i].sectnum;
                    if ((unsigned)index >= MAXSECTORS)
                    {
                        iPlayer = index;
                        insptr++;
                        goto badsector;
                    }
                    return ((VM_AccessSectorX(index, *insptr++) ^ -negateResult) + negateResult);
                case 2: //else if (id == g_iWallVarID)
                    if ((unsigned)index >= MAXWALLS)
                    {
                        iPlayer = index;
                        insptr++;
                        goto badwall;
                    }
                    return ((VM_AccessWallX(index, *insptr++) ^ -negateResult) + negateResult);
                default:
                    goto wtf;
                }
            }

            id &= (MAXGAMEVARS-1);

            if (!negateResult)
                goto badvarid;
        }

        switch (aGameVars[id].dwFlags &
                (GAMEVAR_USER_MASK|GAMEVAR_PTR_MASK))
        {
        default:
            return ((aGameVars[id].val.lValue ^ -negateResult) + negateResult);
        case GAMEVAR_PERPLAYER:
            if ((unsigned)iPlayer >= MAXPLAYERS) goto bad_id;
            return ((aGameVars[id].val.plValues[iPlayer] ^ -negateResult) + negateResult);
        case GAMEVAR_PERACTOR:
            if ((unsigned)iActor >= MAXSPRITES) goto bad_id;
            return ((aGameVars[id].val.plValues[iActor] ^ -negateResult) + negateResult);
        case GAMEVAR_INTPTR:
            return (((*((int32_t *)aGameVars[id].val.lValue)) ^ -negateResult) + negateResult);
        case GAMEVAR_SHORTPTR:
            return (((*((int16_t *)aGameVars[id].val.lValue)) ^ -negateResult) + negateResult);
        case GAMEVAR_CHARPTR:
            return (((*((char *)aGameVars[id].val.lValue)) ^ -negateResult) + negateResult);
        }
    }

bad_id:
    CON_ERRPRINTF("Gv_GetVar(): invalid sprite/player ID %d/%d\n", iActor,iPlayer);
    return -1;

badvarid:
    CON_ERRPRINTF("Gv_GetVar(): invalid gamevar ID (%d)\n", id);
    return -1;

badindex:
    CON_ERRPRINTF("Gv_GetVar(): invalid array index (%s[%d])\n", aGameArrays[id].szLabel,iActor);
    return -1;

badplayer:
    CON_ERRPRINTF("Gv_GetVar(): invalid player ID %d\n", iPlayer);
    return -1;

badsprite:
    CON_ERRPRINTF("Gv_GetVar(): invalid sprite ID %d\n", iPlayer);
    return -1;

badsector:
    CON_ERRPRINTF("Gv_GetVar(): invalid sector ID %d\n", iPlayer);
    return -1;

badwall:
    CON_ERRPRINTF("Gv_GetVar(): invalid wall ID %d\n", iPlayer);
    return -1;

wtf:
    CON_ERRPRINTF("Gv_GetVar(): WTF?\n");
    return -1;
}

void __fastcall Gv_SetVar(register int32_t id, register int32_t lValue, register int32_t iActor, register int32_t iPlayer)
{
    if ((unsigned)id >= (unsigned)g_gameVarCount) goto badvarid;

    //Bsprintf(g_szBuf,"SGVI: %d (\"%s\") to %d for %d %d",id,aGameVars[id].szLabel,lValue,iActor,iPlayer);
    //AddLog(g_szBuf);

    switch (aGameVars[id].dwFlags & (GAMEVAR_USER_MASK|GAMEVAR_PTR_MASK))
    {
    default:
        aGameVars[id].val.lValue=lValue;
        return;
    case GAMEVAR_PERPLAYER:
        if ((unsigned)iPlayer > MAXPLAYERS-1) goto badindex;
        // for the current player
        aGameVars[id].val.plValues[iPlayer]=lValue;
        return;
    case GAMEVAR_PERACTOR:
        if ((unsigned)iActor > MAXSPRITES-1) goto badindex;
        aGameVars[id].val.plValues[iActor]=lValue;
        return;
    case GAMEVAR_INTPTR:
        *((int32_t *)aGameVars[id].val.lValue)=(int32_t)lValue;
        return;
    case GAMEVAR_SHORTPTR:
        *((int16_t *)aGameVars[id].val.lValue)=(int16_t)lValue;
        return;
    case GAMEVAR_CHARPTR:
        *((uint8_t *)aGameVars[id].val.lValue)=(uint8_t)lValue;
        return;
    }

badvarid:
    CON_ERRPRINTF("Gv_SetVar(): invalid gamevar (%d) from sprite %d (%d), player %d\n",
                  id,vm.g_i,TrackerCast(sprite[vm.g_i].picnum),vm.g_p);
    return;

badindex:
    CON_ERRPRINTF("Gv_SetVar(): invalid index (%d) for gamevar %s from sprite %d, player %d\n",
               aGameVars[id].dwFlags & GAMEVAR_PERACTOR ? iActor : iPlayer,
               aGameVars[id].szLabel,vm.g_i,vm.g_p);
    return;
}

int32_t __fastcall Gv_GetVarX(register int32_t id)
{
    if (id == g_iThisActorID)
        return vm.g_i;

    if (id == MAXGAMEVARS)
        return(*insptr++);

    {
        register intptr_t negateResult = id&(MAXGAMEVARS<<1);

        if (id >= g_gameVarCount)
        {
            if (id&(MAXGAMEVARS<<2)) // array
            {
                register int32_t index=Gv_GetVarX(*insptr++);
                int32_t siz;

                id &= (MAXGAMEVARS-1);// ~((MAXGAMEVARS<<2)|(MAXGAMEVARS<<1));

                if (aGameArrays[id].dwFlags & GAMEARRAY_VARSIZE)
                    siz = Gv_GetVarX(aGameArrays[id].size);
                else
                    siz = aGameArrays[id].size;

                if (index < 0 || index >= siz)
                {
                    negateResult = index;
                    goto badindex;
                }

                switch (aGameArrays[id].dwFlags & GAMEARRAY_TYPE_MASK)
                {
                case 0:
                    return ((aGameArrays[id].plValues)[index] ^ -negateResult) + negateResult;
                case GAMEARRAY_OFINT:
                    return (((int32_t *)aGameArrays[id].plValues)[index] ^ -negateResult) + negateResult;
                case GAMEARRAY_OFSHORT:
                    return (((int16_t *)aGameArrays[id].plValues)[index] ^ -negateResult) + negateResult;
                case GAMEARRAY_OFCHAR:
                    return (((uint8_t *)aGameArrays[id].plValues)[index] ^ -negateResult) + negateResult;
                default:
                    goto arraywtf;
                }
            }

            if (id&(MAXGAMEVARS<<3)) // struct shortcut vars
            {
                int32_t index=Gv_GetVarX(*insptr++);

                switch ((id&(MAXGAMEVARS-1)) - g_iSpriteVarID)
                {
                case 0: //if (id == g_iSpriteVarID)
                {
                    register int32_t parm2 = 0, label = *insptr++;

                    /*OSD_Printf("%d %d %d\n",__LINE__,index,label);*/
                    if (ActorLabels[label].flags & LABEL_HASPARM2)
                        parm2 = Gv_GetVarX(*insptr++);

                    if ((unsigned)index >= MAXSPRITES)
                    {
                        id = index;
                        goto badsprite;
                    }

                    return ((VM_AccessSpriteX(index, label, parm2) ^ -negateResult) + negateResult);
                }
                case 3: //else if (id == g_iPlayerVarID)
                {
                    register int32_t parm2 = 0, label = *insptr++;

                    if (PlayerLabels[label].flags & LABEL_HASPARM2)
                        parm2 = Gv_GetVarX(*insptr++);

                    if (index == vm.g_i) index = vm.g_p;

                    if ((unsigned)index >= MAXPLAYERS)
                    {
                        id = index;
                        goto badplayer;
                    }
                    return ((VM_AccessPlayerX(index, label, parm2) ^ -negateResult) + negateResult);
                }
                case 4: //else if (id == g_iActorVarID)
                    return ((Gv_GetVar(*insptr++, index, vm.g_p) ^ -negateResult) + negateResult);
                case 1: //else if (id == g_iSectorVarID)
                    if (index == vm.g_i) index = sprite[vm.g_i].sectnum;
                    if ((unsigned)index >= MAXSECTORS)
                    {
                        id = index;
                        insptr++;
                        goto badsector;
                    }
                    return ((VM_AccessSectorX(index, *insptr++) ^ -negateResult) + negateResult);
                case 2: //else if (id == g_iWallVarID)
                    if ((unsigned)index >= MAXWALLS)
                    {
                        id = index;
                        insptr++;
                        goto badwall;
                    }
                    return ((VM_AccessWallX(index, *insptr++) ^ -negateResult) + negateResult);
                default:
                    goto wtf;
                }
            }

            id &= (MAXGAMEVARS-1);

            if (!negateResult)
                goto badvarid;
        }

        switch (aGameVars[id].dwFlags & (GAMEVAR_USER_MASK|GAMEVAR_PTR_MASK))
        {
        default:
            return ((aGameVars[id].val.lValue ^ -negateResult) + negateResult);
        case GAMEVAR_PERPLAYER:
            if ((unsigned)vm.g_p >= MAXPLAYERS)
            {
                id = vm.g_p;
                goto badplayer;
            }
            return ((aGameVars[id].val.plValues[vm.g_p] ^ -negateResult) + negateResult);
        case GAMEVAR_PERACTOR:
            return ((aGameVars[id].val.plValues[vm.g_i] ^ -negateResult) + negateResult);
        case GAMEVAR_INTPTR:
            return (((*((int32_t *)aGameVars[id].val.lValue)) ^ -negateResult) + negateResult);
        case GAMEVAR_SHORTPTR:
            return (((*((int16_t *)aGameVars[id].val.lValue)) ^ -negateResult) + negateResult);
        case GAMEVAR_CHARPTR:
            return (((*((uint8_t *)aGameVars[id].val.lValue)) ^ -negateResult) + negateResult);
        }

badindex:
        CON_ERRPRINTF("Gv_GetVarX(): invalid array index (%s[%d])\n", aGameArrays[id].szLabel, (int32_t)negateResult);
        return -1;

badvarid:
        CON_ERRPRINTF("Gv_GetVarX(): invalid gamevar ID (%d)\n", id);
        return -1;

badplayer:
        CON_ERRPRINTF("Gv_GetVarX(): invalid player ID %d\n", id);
        return -1;

badsprite:
        CON_ERRPRINTF("Gv_GetVarX(): invalid sprite ID %d\n", id);
        return -1;

badsector:
        CON_ERRPRINTF("Gv_GetVarX(): invalid sector ID %d\n", id);
        return -1;

badwall:
        CON_ERRPRINTF("Gv_GetVarX(): invalid wall ID %d\n", id);
        return -1;

arraywtf:
        CON_ERRPRINTF("Gv_GetVarX() (array): WTF?\n");
        return -1;

wtf:
        CON_ERRPRINTF("Gv_GetVarX(): WTF?\n");
        return -1;
    }
}

void __fastcall Gv_SetVarX(register int32_t id, register int32_t lValue)
{
    switch (aGameVars[id].dwFlags & (GAMEVAR_USER_MASK|GAMEVAR_PTR_MASK))
    {
    default:
        aGameVars[id].val.lValue=lValue;
        return;
    case GAMEVAR_PERPLAYER:
        if ((unsigned)vm.g_p > MAXPLAYERS-1) goto badindex;
        aGameVars[id].val.plValues[vm.g_p]=lValue;
        return;
    case GAMEVAR_PERACTOR:
        if ((unsigned)vm.g_i > MAXSPRITES-1) goto badindex;
        aGameVars[id].val.plValues[vm.g_i]=lValue;
        return;
    case GAMEVAR_INTPTR:
        *((int32_t *)aGameVars[id].val.lValue)=(int32_t)lValue;
        return;
    case GAMEVAR_SHORTPTR:
        *((int16_t *)aGameVars[id].val.lValue)=(int16_t)lValue;
        return;
    case GAMEVAR_CHARPTR:
        *((uint8_t *)aGameVars[id].val.lValue)=(uint8_t)lValue;
        return;
    }

badindex:
    CON_ERRPRINTF("Gv_SetVar(): invalid index (%d) for gamevar %s\n",
               aGameVars[id].dwFlags & GAMEVAR_PERACTOR ? vm.g_i : vm.g_p,
               aGameVars[id].szLabel);
    return;
}

int32_t Gv_GetVarByLabel(const char *szGameLabel, int32_t lDefault, int32_t iActor, int32_t iPlayer)
{
    int32_t i = hash_find(&h_gamevars,szGameLabel);

    if (i < 0)
        return lDefault;

    return Gv_GetVar(i, iActor, iPlayer);
}

static intptr_t *Gv_GetVarDataPtr(const char *szGameLabel)
{
    int32_t i = hash_find(&h_gamevars,szGameLabel);

    if (i < 0)
        return NULL;

    if (aGameVars[i].dwFlags & (GAMEVAR_PERACTOR | GAMEVAR_PERPLAYER))
    {
        if (!aGameVars[i].val.plValues)
            CON_ERRPRINTF("Gv_GetVarDataPtr(): INTERNAL ERROR: NULL array !!!\n");
        return aGameVars[i].val.plValues;
    }

    return &(aGameVars[i].val.lValue);
}
#endif  // !defined LUNATIC

static void G_InitProjectileData(void)
{
    int32_t i;
    for (i=MAXTILES-1; i>=0; i--)
        Bmemcpy(&ProjectileData[i], &g_tile[i].defproj, sizeof(projectile_t));
}

void Gv_ResetSystemDefaults(void)
{
    // call many times...
#if !defined LUNATIC
    int32_t i;
    int32_t j;
    char aszBuf[64];

    //AddLog("ResetWeaponDefaults");

    for (j=MAXPLAYERS-1; j>=0; j--)
    {
        for (i=MAX_WEAPONS-1; i>=0; i--)
        {
            Bsprintf(aszBuf,"WEAPON%d_CLIP",i);
            aplWeaponClip[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_RELOAD",i);
            aplWeaponReload[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",i);
            aplWeaponFireDelay[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",i);
            aplWeaponTotalTime[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",i);
            aplWeaponHoldDelay[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_FLAGS",i);
            aplWeaponFlags[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SHOOTS",i);
            aplWeaponShoots[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            if ((unsigned)aplWeaponShoots[i][j] >= MAXTILES)
                aplWeaponShoots[i][j] = 0;
            Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",i);
            aplWeaponSpawnTime[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SPAWN",i);
            aplWeaponSpawn[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",i);
            aplWeaponShotsPerBurst[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",i);
            aplWeaponWorksLike[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",i);
            aplWeaponInitialSound[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",i);
            aplWeaponFireSound[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",i);
            aplWeaponSound2Time[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",i);
            aplWeaponSound2Sound[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",i);
            aplWeaponReloadSound1[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",i);
            aplWeaponReloadSound2[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SELECTSOUND",i);
            aplWeaponSelectSound[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_FLASHCOLOR",i);
            aplWeaponFlashColor[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
        }
    }

    g_iReturnVarID=Gv_GetVarIndex("RETURN");
    g_iWeaponVarID=Gv_GetVarIndex("WEAPON");
    g_iWorksLikeVarID=Gv_GetVarIndex("WORKSLIKE");
    g_iZRangeVarID=Gv_GetVarIndex("ZRANGE");
    g_iAngRangeVarID=Gv_GetVarIndex("ANGRANGE");
    g_iAimAngleVarID=Gv_GetVarIndex("AUTOAIMANGLE");
    g_iLoTagID=Gv_GetVarIndex("LOTAG");
    g_iHiTagID=Gv_GetVarIndex("HITAG");
    g_iTextureID=Gv_GetVarIndex("TEXTURE");
    g_iThisActorID=Gv_GetVarIndex("THISACTOR");

    g_iSpriteVarID=Gv_GetVarIndex("sprite");
    g_iSectorVarID=Gv_GetVarIndex("sector");
    g_iWallVarID=Gv_GetVarIndex("wall");
    g_iPlayerVarID=Gv_GetVarIndex("player");
    g_iActorVarID=Gv_GetVarIndex("actorvar");
#endif
    G_InitProjectileData();

    //AddLog("EOF:ResetWeaponDefaults");
}

// Will set members that were overridden at CON translation time to 1.
// For example, if
//   gamevar WEAPON1_SHOOTS 2200 GAMEVAR_PERPLAYER
// was specified at file scope, g_weaponOverridden[1].Shoots will be 1.
weapondata_t g_weaponOverridden[MAX_WEAPONS];

static weapondata_t weapondefaults[MAX_WEAPONS] = {
    /*
        WorksLike, Clip, Reload, FireDelay, TotalTime, HoldDelay,
        Flags,
        Shoots, SpawnTime, Spawn, ShotsPerBurst, InitialSound, FireSound, Sound2Time, Sound2Sound,
        ReloadSound1, ReloadSound2, SelectSound, FlashColor
    */

    {
        KNEE_WEAPON, 0, 0, 7, 14, 0,
        WEAPON_NOVISIBLE | WEAPON_RANDOMRESTART | WEAPON_AUTOMATIC,
        KNEE__STATIC, 0, 0, 0, 0, 0, 0,
        0, EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, 0, 0
    },

    {
        PISTOL_WEAPON, /*NAM?20:*/12, /*NAM?50:*/27, 2, 5, 0,
        /*(NAM?WEAPON_HOLSTER_CLEARS_CLIP:0) |*/ WEAPON_RELOAD_TIMING,
        SHOTSPARK1__STATIC, 2, SHELL__STATIC, 0, 0, PISTOL_FIRE__STATIC, 0, 0,
        EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, INSERT_CLIP__STATIC, 255+(95<<8)
    },

    {
        SHOTGUN_WEAPON, 0, 13, 4, 30, 0,
        WEAPON_CHECKATRELOAD,
        SHOTGUN__STATIC, 24, SHOTGUNSHELL__STATIC, 7, 0, SHOTGUN_FIRE__STATIC, 15, SHOTGUN_COCK__STATIC,
        EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, SHOTGUN_COCK__STATIC, 255+(95<<8)
    },

    {
        CHAINGUN_WEAPON, 0, 0, 3, 12, 3,
        WEAPON_AUTOMATIC | WEAPON_FIREEVERYTHIRD | WEAPON_AMMOPERSHOT | WEAPON_SPAWNTYPE3 | WEAPON_RESET,
        CHAINGUN__STATIC, 1, SHELL__STATIC, 0, 0, CHAINGUN_FIRE__STATIC, 0, 0,
        EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, SELECT_WEAPON__STATIC, 255+(95<<8)
    },

    {
        RPG_WEAPON, 0, 0, 4, 20, 0,
        0,
        RPG__STATIC, 0, 0, 0, 0, 0, 0, 0,
        EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, SELECT_WEAPON__STATIC, 255+(95<<8)
    },

    {
        HANDBOMB_WEAPON, 0, 30, 6, 19, 12,
        WEAPON_THROWIT,
        HEAVYHBOMB__STATIC, 0, 0, 0, 0, 0, 0,
        0, EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, 0, 0
    },

    {
        SHRINKER_WEAPON, 0, 0, 10, /*NAM?30:*/12, 0,
        WEAPON_GLOWS,
        SHRINKER__STATIC, 0, 0, 0, SHRINKER_FIRE__STATIC, 0, 0, 0,
        EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, SELECT_WEAPON__STATIC, 128+(255<<8)+(128<<16)
    },

    {
        DEVISTATOR_WEAPON, 0, 0, 3, 6, 5,
        WEAPON_FIREEVERYOTHER | WEAPON_AMMOPERSHOT,
        RPG__STATIC, 0, 0, 2, CAT_FIRE__STATIC, 0, 0, 0,
        EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, SELECT_WEAPON__STATIC, 255+(95<<8)
    },

    {
        TRIPBOMB_WEAPON, 0, 16, 3, 16, 7,
        WEAPON_NOVISIBLE | WEAPON_STANDSTILL | WEAPON_CHECKATRELOAD,
        HANDHOLDINGLASER__STATIC, 0, 0, 0, 0, 0, 0,
        0, EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, 0, 0
    },

    {
        FREEZE_WEAPON, 0, 0, 3, 5, 0,
        WEAPON_RESET,
        FREEZEBLAST__STATIC, 0, 0, 0, CAT_FIRE__STATIC, CAT_FIRE__STATIC, 0, 0,
        EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, SELECT_WEAPON__STATIC, 128+(128<<8)+(255<<16)
    },

    {
        HANDREMOTE_WEAPON, 0, 10, 2, 10, 0,
        WEAPON_BOMB_TRIGGER | WEAPON_NOVISIBLE,
        0, 0, 0, 0, 0, 0, 0,
        0, EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, 0, 0
    },

    {
        GROW_WEAPON, 0, 0, 3, /*NAM?30:*/5, 0,
        WEAPON_GLOWS,
        GROWSPARK__STATIC, /*NAM?2:*/0, /*NAM?SHELL:*/0, 0, 0, /*NAM?0:*/EXPANDERSHOOT__STATIC, 0, 0,
        EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, SELECT_WEAPON__STATIC, 255+(95<<8)
    },
};

// KEEPINSYNC with what is contained above
// XXX: ugly
static int32_t G_StaticToDynamicTile(int32_t tile)
{
    switch (tile)
    {
    case CHAINGUN__STATIC: return CHAINGUN;
    case FREEZEBLAST__STATIC: return FREEZEBLAST;
    case GROWSPARK__STATIC: return GROWSPARK;
    case HANDHOLDINGLASER__STATIC: return HANDHOLDINGLASER;
    case HEAVYHBOMB__STATIC: return HEAVYHBOMB;
    case KNEE__STATIC: return KNEE;
    case RPG__STATIC: return RPG;
    case SHELL__STATIC: return SHELL;
    case SHOTGUNSHELL__STATIC: return SHOTGUNSHELL;
    case SHOTGUN__STATIC: return SHOTGUN;
    case SHOTSPARK1__STATIC: return SHOTSPARK1;
    case SHRINKER__STATIC: return SHRINKER;
    default: return tile;
    }
}
static int32_t G_StaticToDynamicSound(int32_t sound)
{
    switch (sound)
    {
    case CAT_FIRE__STATIC: return CAT_FIRE;
    case CHAINGUN_FIRE__STATIC: return CHAINGUN_FIRE;
    case EJECT_CLIP__STATIC: return EJECT_CLIP;
    case EXPANDERSHOOT__STATIC: return EXPANDERSHOOT;
    case INSERT_CLIP__STATIC: return INSERT_CLIP;
    case PISTOL_FIRE__STATIC: return PISTOL_FIRE;
    case SELECT_WEAPON__STATIC: return SELECT_WEAPON;
    case SHOTGUN_FIRE__STATIC: return SHOTGUN_FIRE;
    case SHOTGUN_COCK__STATIC: return SHOTGUN_COCK;
    case SHRINKER_FIRE__STATIC: return SHRINKER_FIRE;
    default: return sound;
    }
}

// Initialize WEAPONx_* gamevars. Since for Lunatic, they reside on the C side,
// they're set directly. In C-CON, a new CON variable is defined together with
// its initial value.
#ifdef LUNATIC
# define ADDWEAPONVAR(Weapidx, Membname) do { \
    int32_t j; \
    for (j=0; j<MAXPLAYERS; j++) \
        g_playerWeapon[j][Weapidx].Membname = weapondefaults[Weapidx].Membname; \
} while (0)
#else
# define ADDWEAPONVAR(Weapidx, Membname) do { \
    Bsprintf(aszBuf, "WEAPON%d_" #Membname, Weapidx); \
    Bstrupr(aszBuf); \
    Gv_NewVar(aszBuf, weapondefaults[Weapidx].Membname, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM); \
} while (0)
#endif

// After CON translation, get not-overridden members from weapondefaults[] back
// into the live arrays! (That is, g_playerWeapon[][] for Lunatic, WEAPONx_*
// gamevars on the CON side in C-CON.)
#ifdef LUNATIC
# define POSTADDWEAPONVAR(Weapidx, Membname) ADDWEAPONVAR(Weapidx, Membname)
#else
// NYI
# define POSTADDWEAPONVAR(Weapidx, Membname) do {} while (0)
#endif

// Finish a default weapon member after CON translation. If it was not
// overridden from CON itself (see example at g_weaponOverridden[]), we set
// both the weapondefaults[] entry (probably dead by now) and the live value.
#define FINISH_WEAPON_DEFAULT_X(What, i, Membname) do {  \
    if (!g_weaponOverridden[i].Membname) \
    { \
        weapondefaults[i].Membname = G_StaticToDynamic##What(weapondefaults[i].Membname); \
        POSTADDWEAPONVAR(i, Membname); \
    } \
} while (0)

#define FINISH_WEAPON_DEFAULT_TILE(i, Membname) FINISH_WEAPON_DEFAULT_X(Tile, i, Membname)
#define FINISH_WEAPON_DEFAULT_SOUND(i, Membname) FINISH_WEAPON_DEFAULT_X(Sound, i, Membname)

// Process the dynamic {tile,sound} mappings after CON has been translated.
// We cannot do this before, because the dynamic maps are not yet set up then.
void Gv_FinalizeWeaponDefaults(void)
{
    int32_t i;

    for (i=0; i<MAX_WEAPONS; i++)
    {
        FINISH_WEAPON_DEFAULT_TILE(i, Shoots);
        FINISH_WEAPON_DEFAULT_TILE(i, Spawn);

        FINISH_WEAPON_DEFAULT_SOUND(i, InitialSound);
        FINISH_WEAPON_DEFAULT_SOUND(i, FireSound);
        FINISH_WEAPON_DEFAULT_SOUND(i, ReloadSound1);
        FINISH_WEAPON_DEFAULT_SOUND(i, Sound2Sound);
        FINISH_WEAPON_DEFAULT_SOUND(i, ReloadSound2);
        FINISH_WEAPON_DEFAULT_SOUND(i, SelectSound);
    }
}
#undef FINISH_WEAPON_DEFAULT_SOUND
#undef FINISH_WEAPON_DEFAULT_TILE
#undef FINISH_WEAPON_DEFAULT_X
#undef POSTADDWEAPONVAR

static void Gv_AddSystemVars(void)
{
    // only call ONCE
#if !defined LUNATIC
    char aszBuf[64];
#endif
    int32_t i;

    if (NAM)
    {
        weapondefaults[PISTOL_WEAPON].Clip = 20;
        weapondefaults[PISTOL_WEAPON].Reload = 50;
        weapondefaults[PISTOL_WEAPON].Flags |= WEAPON_HOLSTER_CLEARS_CLIP;

        weapondefaults[SHRINKER_WEAPON].TotalTime = 30;

        weapondefaults[GROW_WEAPON].TotalTime = 30;
        weapondefaults[GROW_WEAPON].SpawnTime = 2;
        weapondefaults[GROW_WEAPON].Spawn = SHELL;
        weapondefaults[GROW_WEAPON].FireSound = 0;
    }

    for (i=0; i<MAX_WEAPONS; i++)
    {
        ADDWEAPONVAR(i, WorksLike);
        ADDWEAPONVAR(i, Clip);
        ADDWEAPONVAR(i, Reload);
        ADDWEAPONVAR(i, FireDelay);
        ADDWEAPONVAR(i, TotalTime);
        ADDWEAPONVAR(i, HoldDelay);
        ADDWEAPONVAR(i, Flags);
        ADDWEAPONVAR(i, Shoots);
        ADDWEAPONVAR(i, SpawnTime);
        ADDWEAPONVAR(i, Spawn);
        ADDWEAPONVAR(i, ShotsPerBurst);
        ADDWEAPONVAR(i, InitialSound);
        ADDWEAPONVAR(i, FireSound);
        ADDWEAPONVAR(i, Sound2Time);
        ADDWEAPONVAR(i, Sound2Sound);
        ADDWEAPONVAR(i, ReloadSound1);
        ADDWEAPONVAR(i, ReloadSound2);
        ADDWEAPONVAR(i, SelectSound);
        ADDWEAPONVAR(i, FlashColor);
    }
#ifdef LUNATIC
    for (i=0; i<MAXPLAYERS; i++)
    {
        DukePlayer_t *ps = g_player[i].ps;

        ps->pipebombControl = NAM ? PIPEBOMB_TIMER : PIPEBOMB_REMOTE;
        ps->pipebombLifetime = NAM_GRENADE_LIFETIME;
        ps->pipebombLifetimeVar = NAM_GRENADE_LIFETIME_VAR;

        ps->tripbombControl = TRIPBOMB_TRIPWIRE;
        ps->tripbombLifetime = NAM_GRENADE_LIFETIME;
        ps->tripbombLifetimeVar = NAM_GRENADE_LIFETIME_VAR;
    }
#else
    Gv_NewVar("GRENADE_LIFETIME", NAM_GRENADE_LIFETIME, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Gv_NewVar("GRENADE_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    Gv_NewVar("STICKYBOMB_LIFETIME", NAM_GRENADE_LIFETIME, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Gv_NewVar("STICKYBOMB_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    Gv_NewVar("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Gv_NewVar("PIPEBOMB_CONTROL", NAM?PIPEBOMB_TIMER:PIPEBOMB_REMOTE, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    Gv_NewVar("RESPAWN_MONSTERS", (intptr_t)&ud.respawn_monsters,GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("RESPAWN_ITEMS",(intptr_t)&ud.respawn_items, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("RESPAWN_INVENTORY",(intptr_t)&ud.respawn_inventory, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("MONSTERS_OFF",(intptr_t)&ud.monsters_off, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("MARKER",(intptr_t)&ud.marker, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("FFIRE",(intptr_t)&ud.ffire, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("LEVEL",(intptr_t)&ud.level_number, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("VOLUME",(intptr_t)&ud.volume_number, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);

    Gv_NewVar("COOP",(intptr_t)&ud.coop, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("MULTIMODE",(intptr_t)&ud.multimode, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);

    Gv_NewVar("WEAPON", 0, GAMEVAR_PERPLAYER | GAMEVAR_READONLY | GAMEVAR_SYSTEM);
    Gv_NewVar("WORKSLIKE", 0, GAMEVAR_PERPLAYER | GAMEVAR_READONLY | GAMEVAR_SYSTEM);
    Gv_NewVar("RETURN", 0, GAMEVAR_SYSTEM);
    Gv_NewVar("ZRANGE", 4, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Gv_NewVar("ANGRANGE", 18, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Gv_NewVar("AUTOAIMANGLE", 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Gv_NewVar("LOTAG", 0, GAMEVAR_SYSTEM);
    Gv_NewVar("HITAG", 0, GAMEVAR_SYSTEM);
    Gv_NewVar("TEXTURE", 0, GAMEVAR_SYSTEM);
    Gv_NewVar("THISACTOR", 0, GAMEVAR_READONLY | GAMEVAR_SYSTEM);

    // special vars for struct access
    Gv_NewVar("sprite", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("sector", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("wall", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("player", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("actorvar", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);

    Gv_NewVar("myconnectindex", (intptr_t)&myconnectindex, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("screenpeek", (intptr_t)&screenpeek, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("currentweapon",(intptr_t)&hudweap.cur, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("gs",(intptr_t)&hudweap.shade, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("looking_arc",(intptr_t)&hudweap.lookhoriz, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("gun_pos",(intptr_t)&hudweap.gunposy, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("weapon_xoffset",(intptr_t)&hudweap.gunposx, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("weaponcount",(intptr_t)&hudweap.count, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("looking_angSR1",(intptr_t)&hudweap.lookhalfang, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("xdim",(intptr_t)&xdim, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("ydim",(intptr_t)&ydim, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("windowx1",(intptr_t)&windowx1, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("windowx2",(intptr_t)&windowx2, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("windowy1",(intptr_t)&windowy1, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("windowy2",(intptr_t)&windowy2, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("totalclock",(intptr_t)&totalclock, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("lastvisinc",(intptr_t)&lastvisinc, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("numsectors",(intptr_t)&numsectors, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_READONLY);

    Gv_NewVar("current_menu",(intptr_t)&g_currentMenu, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("numplayers",(intptr_t)&numplayers, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("viewingrange",(intptr_t)&viewingrange, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("yxaspect",(intptr_t)&yxaspect, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("gravitationalconstant",(intptr_t)&g_spriteGravity, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("gametype_flags",(intptr_t)&GametypeFlags[ud.coop], GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("framerate",(intptr_t)&g_currentFrameRate, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("CLIPMASK0", CLIPMASK0, GAMEVAR_SYSTEM|GAMEVAR_READONLY);
    Gv_NewVar("CLIPMASK1", CLIPMASK1, GAMEVAR_SYSTEM|GAMEVAR_READONLY);

    Gv_NewVar("camerax",(intptr_t)&ud.camerapos.x, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("cameray",(intptr_t)&ud.camerapos.y, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("cameraz",(intptr_t)&ud.camerapos.z, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("cameraang",(intptr_t)&ud.cameraang, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR);
    Gv_NewVar("camerahoriz",(intptr_t)&ud.camerahoriz, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR);
    Gv_NewVar("camerasect",(intptr_t)&ud.camerasect, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR);
    Gv_NewVar("cameradist",(intptr_t)&g_cameraDistance, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("cameraclock",(intptr_t)&g_cameraClock, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);

    Gv_NewVar("myx",(intptr_t)&my.x, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("myy",(intptr_t)&my.y, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("myz",(intptr_t)&my.z, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("omyx",(intptr_t)&omy.x, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("omyy",(intptr_t)&omy.y, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("omyz",(intptr_t)&omy.z, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("myvelx",(intptr_t)&myvel.x, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("myvely",(intptr_t)&myvel.y, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("myvelz",(intptr_t)&myvel.z, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);

    Gv_NewVar("myhoriz",(intptr_t)&myhoriz, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR);
    Gv_NewVar("myhorizoff",(intptr_t)&myhorizoff, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR);
    Gv_NewVar("omyhoriz",(intptr_t)&omyhoriz, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR);
    Gv_NewVar("omyhorizoff",(intptr_t)&omyhorizoff, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR);
    Gv_NewVar("myang",(intptr_t)&myang, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR);
    Gv_NewVar("omyang",(intptr_t)&omyang, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR);
    Gv_NewVar("mycursectnum",(intptr_t)&mycursectnum, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR);
    Gv_NewVar("myjumpingcounter",(intptr_t)&myjumpingcounter, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR);

    Gv_NewVar("myjumpingtoggle",(intptr_t)&myjumpingtoggle, GAMEVAR_SYSTEM | GAMEVAR_CHARPTR);
    Gv_NewVar("myonground",(intptr_t)&myonground, GAMEVAR_SYSTEM | GAMEVAR_CHARPTR);
    Gv_NewVar("myhardlanding",(intptr_t)&myhardlanding, GAMEVAR_SYSTEM | GAMEVAR_CHARPTR);
    Gv_NewVar("myreturntocenter",(intptr_t)&myreturntocenter, GAMEVAR_SYSTEM | GAMEVAR_CHARPTR);

    Gv_NewVar("display_mirror",(intptr_t)&display_mirror, GAMEVAR_SYSTEM | GAMEVAR_CHARPTR);
    Gv_NewVar("randomseed",(intptr_t)&randomseed, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);

    Gv_NewVar("NUMWALLS",(intptr_t)&numwalls, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_READONLY);
    Gv_NewVar("NUMSECTORS",(intptr_t)&numsectors, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_READONLY);
    Gv_NewVar("Numsprites",(intptr_t)&Numsprites, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);

    Gv_NewVar("lastsavepos",(intptr_t)&g_lastSaveSlot, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
# ifdef USE_OPENGL
    Gv_NewVar("rendmode",(intptr_t)&rendmode, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
# else
    Gv_NewVar("rendmode", 0, GAMEVAR_READONLY | GAMEVAR_SYSTEM);
# endif

    Gv_NewArray("tilesizx", (void *)tilesizx, MAXTILES, GAMEARRAY_READONLY|GAMEARRAY_OFSHORT);
    Gv_NewArray("tilesizy", (void *)tilesizy, MAXTILES, GAMEARRAY_READONLY|GAMEARRAY_OFSHORT);
#endif
}

#undef ADDWEAPONVAR

void Gv_Init(void)
{
    // only call ONCE

    // (... not true anymore)
    static int32_t inited=0;
    if (inited)
        return;
    inited = 1;

#if !defined LUNATIC
    Gv_Clear();
#endif
    // Set up weapon defaults, g_playerWeapon[][].
    Gv_AddSystemVars();
#if !defined LUNATIC
    Gv_InitWeaponPointers();
#endif
    Gv_ResetSystemDefaults();
}

#if !defined LUNATIC
void Gv_InitWeaponPointers(void)
{
    int32_t i;
    char aszBuf[64];
    // called from game Init AND when level is loaded...

    //AddLog("Gv_InitWeaponPointers");

    for (i=(MAX_WEAPONS-1); i>=0; i--)
    {
        Bsprintf(aszBuf,"WEAPON%d_CLIP",i);
        aplWeaponClip[i]=Gv_GetVarDataPtr(aszBuf);
        if (!aplWeaponClip[i])
        {
            initprintf("ERROR: NULL weapon!  WTF?!\n");
            // exit(0);
            G_Shutdown();
        }
        Bsprintf(aszBuf,"WEAPON%d_RELOAD",i);
        aplWeaponReload[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",i);
        aplWeaponFireDelay[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",i);
        aplWeaponTotalTime[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",i);
        aplWeaponHoldDelay[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_FLAGS",i);
        aplWeaponFlags[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SHOOTS",i);
        aplWeaponShoots[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",i);
        aplWeaponSpawnTime[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SPAWN",i);
        aplWeaponSpawn[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",i);
        aplWeaponShotsPerBurst[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",i);
        aplWeaponWorksLike[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",i);
        aplWeaponInitialSound[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",i);
        aplWeaponFireSound[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",i);
        aplWeaponSound2Time[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",i);
        aplWeaponSound2Sound[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",i);
        aplWeaponReloadSound1[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",i);
        aplWeaponReloadSound2[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SELECTSOUND",i);
        aplWeaponSelectSound[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_FLASHCOLOR",i);
        aplWeaponFlashColor[i]=Gv_GetVarDataPtr(aszBuf);
    }
}

void Gv_RefreshPointers(void)
{
    aGameVars[Gv_GetVarIndex("RESPAWN_MONSTERS")].val.lValue = (intptr_t)&ud.respawn_monsters;
    aGameVars[Gv_GetVarIndex("RESPAWN_ITEMS")].val.lValue = (intptr_t)&ud.respawn_items;
    aGameVars[Gv_GetVarIndex("RESPAWN_INVENTORY")].val.lValue = (intptr_t)&ud.respawn_inventory;
    aGameVars[Gv_GetVarIndex("MONSTERS_OFF")].val.lValue = (intptr_t)&ud.monsters_off;
    aGameVars[Gv_GetVarIndex("MARKER")].val.lValue = (intptr_t)&ud.marker;
    aGameVars[Gv_GetVarIndex("FFIRE")].val.lValue = (intptr_t)&ud.ffire;
    aGameVars[Gv_GetVarIndex("LEVEL")].val.lValue = (intptr_t)&ud.level_number;
    aGameVars[Gv_GetVarIndex("VOLUME")].val.lValue = (intptr_t)&ud.volume_number;

    aGameVars[Gv_GetVarIndex("COOP")].val.lValue = (intptr_t)&ud.coop;
    aGameVars[Gv_GetVarIndex("MULTIMODE")].val.lValue = (intptr_t)&ud.multimode;

    aGameVars[Gv_GetVarIndex("myconnectindex")].val.lValue = (intptr_t)&myconnectindex;
    aGameVars[Gv_GetVarIndex("screenpeek")].val.lValue = (intptr_t)&screenpeek;
    aGameVars[Gv_GetVarIndex("currentweapon")].val.lValue = (intptr_t)&hudweap.cur;
    aGameVars[Gv_GetVarIndex("gs")].val.lValue = (intptr_t)&hudweap.shade;
    aGameVars[Gv_GetVarIndex("looking_arc")].val.lValue = (intptr_t)&hudweap.lookhoriz;
    aGameVars[Gv_GetVarIndex("gun_pos")].val.lValue = (intptr_t)&hudweap.gunposy;
    aGameVars[Gv_GetVarIndex("weapon_xoffset")].val.lValue = (intptr_t)&hudweap.gunposx;
    aGameVars[Gv_GetVarIndex("weaponcount")].val.lValue = (intptr_t)&hudweap.count;
    aGameVars[Gv_GetVarIndex("looking_angSR1")].val.lValue = (intptr_t)&hudweap.lookhalfang;
    aGameVars[Gv_GetVarIndex("xdim")].val.lValue = (intptr_t)&xdim;
    aGameVars[Gv_GetVarIndex("ydim")].val.lValue = (intptr_t)&ydim;
    aGameVars[Gv_GetVarIndex("windowx1")].val.lValue = (intptr_t)&windowx1;
    aGameVars[Gv_GetVarIndex("windowx2")].val.lValue = (intptr_t)&windowx2;
    aGameVars[Gv_GetVarIndex("windowy1")].val.lValue = (intptr_t)&windowy1;
    aGameVars[Gv_GetVarIndex("windowy2")].val.lValue = (intptr_t)&windowy2;
    aGameVars[Gv_GetVarIndex("totalclock")].val.lValue = (intptr_t)&totalclock;
    aGameVars[Gv_GetVarIndex("lastvisinc")].val.lValue = (intptr_t)&lastvisinc;
    aGameVars[Gv_GetVarIndex("numsectors")].val.lValue = (intptr_t)&numsectors;
    aGameVars[Gv_GetVarIndex("numplayers")].val.lValue = (intptr_t)&numplayers;
    aGameVars[Gv_GetVarIndex("current_menu")].val.lValue = (intptr_t)&g_currentMenu;
    aGameVars[Gv_GetVarIndex("viewingrange")].val.lValue = (intptr_t)&viewingrange;
    aGameVars[Gv_GetVarIndex("yxaspect")].val.lValue = (intptr_t)&yxaspect;
    aGameVars[Gv_GetVarIndex("gravitationalconstant")].val.lValue = (intptr_t)&g_spriteGravity;
    aGameVars[Gv_GetVarIndex("gametype_flags")].val.lValue = (intptr_t)&GametypeFlags[ud.coop];
    aGameVars[Gv_GetVarIndex("framerate")].val.lValue = (intptr_t)&g_currentFrameRate;

    aGameVars[Gv_GetVarIndex("camerax")].val.lValue = (intptr_t)&ud.camerapos.x;
    aGameVars[Gv_GetVarIndex("cameray")].val.lValue = (intptr_t)&ud.camerapos.y;
    aGameVars[Gv_GetVarIndex("cameraz")].val.lValue = (intptr_t)&ud.camerapos.z;
    aGameVars[Gv_GetVarIndex("cameraang")].val.lValue = (intptr_t)&ud.cameraang;
    aGameVars[Gv_GetVarIndex("camerahoriz")].val.lValue = (intptr_t)&ud.camerahoriz;
    aGameVars[Gv_GetVarIndex("camerasect")].val.lValue = (intptr_t)&ud.camerasect;
    aGameVars[Gv_GetVarIndex("cameradist")].val.lValue = (intptr_t)&g_cameraDistance;
    aGameVars[Gv_GetVarIndex("cameraclock")].val.lValue = (intptr_t)&g_cameraClock;

    aGameVars[Gv_GetVarIndex("myx")].val.lValue = (intptr_t)&my.x;
    aGameVars[Gv_GetVarIndex("myy")].val.lValue = (intptr_t)&my.y;
    aGameVars[Gv_GetVarIndex("myz")].val.lValue = (intptr_t)&my.z;
    aGameVars[Gv_GetVarIndex("omyx")].val.lValue = (intptr_t)&omy.x;
    aGameVars[Gv_GetVarIndex("omyy")].val.lValue = (intptr_t)&omy.y;
    aGameVars[Gv_GetVarIndex("omyz")].val.lValue = (intptr_t)&omy.z;
    aGameVars[Gv_GetVarIndex("myvelx")].val.lValue = (intptr_t)&myvel.x;
    aGameVars[Gv_GetVarIndex("myvely")].val.lValue = (intptr_t)&myvel.y;
    aGameVars[Gv_GetVarIndex("myvelz")].val.lValue = (intptr_t)&myvel.z;

    aGameVars[Gv_GetVarIndex("myhoriz")].val.lValue = (intptr_t)&myhoriz;
    aGameVars[Gv_GetVarIndex("myhorizoff")].val.lValue = (intptr_t)&myhorizoff;
    aGameVars[Gv_GetVarIndex("omyhoriz")].val.lValue = (intptr_t)&omyhoriz;
    aGameVars[Gv_GetVarIndex("omyhorizoff")].val.lValue = (intptr_t)&omyhorizoff;
    aGameVars[Gv_GetVarIndex("myang")].val.lValue = (intptr_t)&myang;
    aGameVars[Gv_GetVarIndex("omyang")].val.lValue = (intptr_t)&omyang;
    aGameVars[Gv_GetVarIndex("mycursectnum")].val.lValue = (intptr_t)&mycursectnum;
    aGameVars[Gv_GetVarIndex("myjumpingcounter")].val.lValue = (intptr_t)&myjumpingcounter;

    aGameVars[Gv_GetVarIndex("myjumpingtoggle")].val.lValue = (intptr_t)&myjumpingtoggle;
    aGameVars[Gv_GetVarIndex("myonground")].val.lValue = (intptr_t)&myonground;
    aGameVars[Gv_GetVarIndex("myhardlanding")].val.lValue = (intptr_t)&myhardlanding;
    aGameVars[Gv_GetVarIndex("myreturntocenter")].val.lValue = (intptr_t)&myreturntocenter;

    aGameVars[Gv_GetVarIndex("display_mirror")].val.lValue = (intptr_t)&display_mirror;
    aGameVars[Gv_GetVarIndex("randomseed")].val.lValue = (intptr_t)&randomseed;

    aGameVars[Gv_GetVarIndex("NUMWALLS")].val.lValue = (intptr_t)&numwalls;
    aGameVars[Gv_GetVarIndex("NUMSECTORS")].val.lValue = (intptr_t)&numsectors;
    aGameVars[Gv_GetVarIndex("Numsprites")].val.lValue = (intptr_t)&Numsprites;

    aGameVars[Gv_GetVarIndex("lastsavepos")].val.lValue = (intptr_t)&g_lastSaveSlot;
# ifdef USE_OPENGL
    aGameVars[Gv_GetVarIndex("rendmode")].val.lValue = (intptr_t)&rendmode;
# endif
}
#endif
